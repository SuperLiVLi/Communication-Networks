//
// Created by li Vlon on 10/13/23.
//
/*
 * File:   sender_main.c
 * Author:
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/time.h>
#include <string>
#include <queue>
#include <math.h>
#include <ctime>
#include <cstring>
#include <algorithm>

using namespace std;
#define MSS 1200
#define Slow_start 0
#define Congestion_avoidance 1
#define Fast_recovery 2
#define time_out 30000

FILE *fp;
int state_flag;
struct sockaddr_in si_other; // IPv4 address structure
int s, slen; // s:socket var  slen: length of client socket
float rwnd; // congestion window and receiver's window buffer
float cwnd = MSS;
float ssthresh;
long lastAck = 0;
int duplicateAck = 0;
double estimatedtime;
uint64_t seq_num;
uint64_t bytes_to_transfer;

enum segment_flag{
    NORMAL,
    ACK,
    FINACK,
    FIN
};

typedef struct Segment{
    long seq_num;
    long return_ack;
    short len;
    char ack_flag; // used to store "segment type" to disconnect
    char datagram[MSS];
}segment;


queue<Segment> queue_wait_ack;  // the segments in cwnd which wait ACK
queue<Segment> avai_send; // the spare space in cwnd which can used to send new segments

void diep(char *s) {
    perror(s);
    exit(1);
}

void send_seg(segment pkt){
    // cout << "Sending packet " << pkt->seq_idx << endl;
    if (sendto(s, &pkt, sizeof(segment), 0, (sockaddr*)&si_other, sizeof(si_other))== -1){
        diep("sendto()");
    }
}

void fetch_and_enqueue(){
    if (bytes_to_transfer == 0){
        return;
    }
    char buffer[MSS];
    memset(buffer,0,MSS);

    // enqueue
    for(int i=0;i<ceil((cwnd-queue_wait_ack.size()*MSS)/MSS);i++){
        int temp = min(bytes_to_transfer, (uint64_t)MSS);
        ::printf("\n\n\nloop:: %f\n\n\n",(cwnd-queue_wait_ack.size()*MSS)/MSS);
        printf("\n\n\ntemp:%d\n\n\n",temp);
        memset(buffer,0,MSS);
        int readLength =fread(buffer, sizeof(char), temp, fp);
        if(readLength>0) {
            segment seg;
            // initialize each segment
            seg.ack_flag = NORMAL;
            seg.len = readLength;
            seg.seq_num = seq_num;
            seg.return_ack = seq_num + seg.len;
            memcpy(seg.datagram, buffer, readLength);
            //fwrite(seg.datagram,sizeof(char),sizeof(seg.datagram),fff);
            //push into queue
            //printf("%s\n",seg.datagram);
            queue_wait_ack.push(seg);
            avai_send.push(seg);
            seq_num+=readLength;
            bytes_to_transfer-=readLength;
            //printf("\n%llu\n",bytes_to_transfer);
        }

    }
    // send segments to receiver
    while(!avai_send.empty()){
        send_seg(avai_send.front());
        printf("\n\n\nsend datagram:%s\n\n\n",avai_send.front().datagram);
        avai_send.pop();
    }
}

void state_handler(long ack){
    // mode1: Slow-start
    if(state_flag == Slow_start){
        if (ack > lastAck){
            duplicateAck = 0;
            cwnd += MSS;
            lastAck = ack;
            while(!queue_wait_ack.empty() && queue_wait_ack.front().seq_num < ack){
                queue_wait_ack.pop();
            }
        }else if(ack == lastAck){
            duplicateAck ++;
        }
        if(duplicateAck == 3){
            ssthresh = cwnd/2;
            cwnd = ssthresh + 3 * MSS;
            state_flag = Fast_recovery;
        }
        if(cwnd >= ssthresh){
            state_flag = Congestion_avoidance;
        }
        return;

    //mode2: Congestion-avoidance
    }else if(state_flag == Congestion_avoidance){
        if(ack > lastAck){
            cwnd += MSS*(MSS/cwnd);
            duplicateAck = 0;
            lastAck = ack;
            while(!queue_wait_ack.empty() && queue_wait_ack.front().seq_num < ack){
                queue_wait_ack.pop();
            }
        }else if(ack == lastAck){
            duplicateAck ++;
        }
        if(duplicateAck == 3){
            ssthresh = cwnd/2;
            cwnd = ssthresh + 3 * MSS;
            state_flag = Fast_recovery;
        }
        return;

    // mode3: Fast-recovery
    }else{
        if(ack > lastAck){
            cwnd = ssthresh;
            duplicateAck = 0;
            state_flag = Congestion_avoidance;
            lastAck = ack;
            while(!queue_wait_ack.empty() && queue_wait_ack.front().seq_num < ack){
                queue_wait_ack.pop();
            }
        }else if(ack == lastAck){
            cwnd = cwnd + MSS;
        }
    }
}


void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

    /* Determine how many bytes to transfer */
    slen= sizeof(si_other);
    int total_segment=ceil(bytesToTransfer/MSS);//ensure no left that wasn't transfered

    /* construct UDP socket connection */
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket fail");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    /* Send data and receive acknowledgements on s*/
    seq_num=0;
    state_flag=0;
    segment seg;
    estimatedtime=0;
    ssthresh=10240;
    bytes_to_transfer=bytesToTransfer;

    /*Set timeout for state trans*/
    timeval RTT;
    RTT.tv_sec=0;
    RTT.tv_usec=time_out;
    if(setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&RTT,sizeof(RTT))==-1)
        diep("Socket");

    fetch_and_enqueue();

    // send and state handler
    while(!queue_wait_ack.empty() && bytes_to_transfer>0){
        if(recvfrom(s,&seg, sizeof(segment),0,NULL,NULL)==-1){
            if (errno != EAGAIN || errno != EWOULDBLOCK) {
                diep("recvfrom()");
            }
            ssthresh = cwnd / 2;
            cwnd = MSS;
            duplicateAck = 0;
            state_flag = Slow_start;
            queue<segment> resend_timeout=queue_wait_ack;

            while(!resend_timeout.empty()){
                send_seg(resend_timeout.front());
                resend_timeout.pop();
            }
        }

        else{
            if(seg.ack_flag==NORMAL) {
                state_handler(seg.seq_num);
                fetch_and_enqueue();
            }
        }
    }

    //end connection
    segment seg1;
    seg1.ack_flag=FIN;
    seg1.seq_num=0;
    seg1.return_ack=0;
    memset(seg1.datagram,0,MSS);
    int fin_ack=0;

    while(true){
        if(fin_ack==2) {
            fclose(fp);
            printf("Closing the socket\n");
            close(s);
            break;
        }
        if(sendto(s,&seg1,sizeof(segment),0,(sockaddr*)&si_other,sizeof(si_other))==-1)
            diep("Send FIN fail");
        else{
            if(recvfrom(s,&seg, sizeof(segment),0,NULL,NULL)==-1){
                diep("Receive FINACK fail");
            }
            else if(seg.ack_flag==FINACK){
                fin_ack++;
            }
        }
    }
    return;

}

/*
 *
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);


    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);

    return (EXIT_SUCCESS);
}



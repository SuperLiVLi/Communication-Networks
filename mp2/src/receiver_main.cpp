/*
 * File:   receiver_main.c
 * Author:
 *
 * Created on
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <unordered_set>
#define MSS 1200

char datagram[MSS];
using namespace std;

enum segment_flag{
    NORMAL,
    ACK,
    FINACK,
    FIN
};

struct sockaddr_in si_me, si_other;
int s, slen;
uint64_t ack_num;
int fin_dup=0;
uint64_t recv_num;
unordered_set<long> hashSet;

typedef struct Segment{
    long seq_num;
    long return_ack;
    short len;
    char ack_flag; // used to store "segment type" to disconnect
    char datagram[MSS];
}segment;

queue<Segment> readbuffer;

void diep(char *s) {
    perror(s);
    exit(1);
}

void send_ack(int ack_idx, segment_flag ack_type){
    segment seg;
    seg.ack_flag=ack_type;
    seg.seq_num=ack_idx;

    if (sendto(s, &seg, sizeof(segment), 0, (sockaddr*)&si_other, (socklen_t)sizeof (si_other))==-1){
        diep("A");
    }
}

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {

    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");
    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (::bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");

    /* Now receive data and send acknowledgements */
    FILE *fp;
    fp= fopen(destinationFile,"wb");
    if(fp==NULL){
        diep("Open file fail");
    }

    // initialize vars
    ack_num=0;

    while(true){
        segment recv_seg;
        if(recvfrom(s,&recv_seg,sizeof(segment),0,(sockaddr*)&si_other,(socklen_t*)&slen)==-1){
            diep("Receivefrom()");
        }
        else{
            if(fin_dup==2){
                printf("CLOSE CONNECTION");
                break;
            }
            if(recv_seg.ack_flag==FIN) {
                send_ack(ack_num, FINACK);
                printf("FIN received.");
                fin_dup++;
            }
            else if(recv_seg.ack_flag==NORMAL){
                if(recv_seg.seq_num<ack_num){
                }
                else if(recv_seg.seq_num==ack_num){
                    if(hashSet.find(recv_seg.seq_num) == hashSet.end()){
                        hashSet.insert(recv_seg.seq_num);
                        fwrite(recv_seg.datagram,sizeof(char),recv_seg.len,fp);
                        //fputs(recv_seg.datagram,fp);
                        printf("seq_num: %ld", recv_seg.seq_num);
                        printf("ack_num: %llu", ack_num);

                        ack_num+=recv_seg.len;
                        printf("\nack_num_inc: %llu\n\n", ack_num);
                        printf("%s",recv_seg.datagram);
                    }
                }
                send_ack(ack_num+recv_seg.len, NORMAL);
            }
        }
    }
    close(s);
    printf("%s received.", destinationFile);
    return;
}

/*
 *
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);
    reliablyReceive(udpPort, argv[2]);
}


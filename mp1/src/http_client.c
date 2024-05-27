/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

//#define PORT "80"  // the port users will be connecting to

#define MAXDATASIZE 4096 // max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int parse(const char *url, char *head, char *name, char *port, char *path)
{
    int i, q, h, j;

    // assign "http://"
    strncpy(head, url, 7);
    head[7] = '\0';  // 添加字符串终止符

    // assign hostname
    q = 7;
    for (i = q; i < strlen(url); i++) {
        if (url[i] == '/' || url[i] == ':') {
            break;
        }
    }
    strncpy(name, url + 7, i - q);
    name[i - q] = '\0';  // 添加字符串终止符


    h = i;
    if(url[i]==':'){
        // assign port

        for (j = h; j < strlen(url); j++) {
            if (url[j] == '/') {
                break;
            }
        }
        strncpy(port, url + i + 1, j - h - 1);
        port[j - h - 1] = '\0';  // 添加字符串终止符
    }
    else {
        port[0]='8';
        port[1]='0';
        port[2]='\0';
        j=h;
    }

    // assign path_to_file
    strncpy(path, url + j, strlen(url) - j);
    path[strlen(url) - j] = '\0';  // 添加字符串终止符

    return 0;

}

int main(int argc, char *argv[])
{

	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

    char request[200];

    char head[8];
    char name[100];
    char port[8];
    char path[100];

    FILE *fp=fopen("output","wb" );

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;


    //parse the url to four parts
    parse(argv[1],head,name,port,path);


	if ((rv = getaddrinfo(name, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);


	freeaddrinfo(servinfo); // all done with this structure



/*	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}*/

    // Construct Get type http request
    sprintf(request, "GET %s HTTP/1.1\r\n"
                     "User-Agent: Wget/1.12 (linux-gnu)\r\n"
                     "Host: %s:%s\r\n"
                     "Connection: keep-Alive\r\n"
                     "\r\n", path, name, port);

    //send REQUEST content to server
    ssize_t bytes_sent = send(sockfd, request, strlen(request), 0);
    if (bytes_sent == -1) {
        perror("send");
        exit(1);
    }
    printf("Send success\n");

 /*   for(int i=0;;i++){
        char response[MAXDATASIZE];
        ssize_t bytes_received = recv(sockfd, response, MAXDATASIZE - 1, 0);
        if (bytes_received == -1) {
            perror("recv");
            break;
        }
        response[bytes_received] = '\0';
        //print output
        printf("%s\n", response);

        if(i>0)
        // write output in file
            fputs(response, fp);

        if(strlen(response)<2047){
            break;
        }

    }*/



    int header_end_found = 0;
    char *body_start = NULL;

    for(int i=0;;i++){
        char response[MAXDATASIZE];
        ssize_t bytes_received = recv(sockfd, response, MAXDATASIZE - 1, 0);
        if (bytes_received == -1) {
            perror("recv");
            break;
        }
        response[bytes_received] = '\0';

        printf("%s\n", response); // Uncomment if you want to print the response to
        printf("\n\n\n\n");

            //printf("\n\n\n\n");
        if (!header_end_found) {
            body_start = strstr(response, "\r\n\r\n");
            if (body_start) {
                body_start += 4; // Skip the \r\n\r\n
                header_end_found = 1;

                // 从body开始位置写入文件
                fputs(body_start, fp);
            }
        } else {
            fputs(response, fp);
            // 一种简单的方式来检测文件是否接收完毕（这取决于服务器如何结束响应）
            if( i>30 && strlen(response) < MAXDATASIZE - 2){
                break;
            }
        }


    }

    fclose(fp);
	close(sockfd);
	return 0;
}


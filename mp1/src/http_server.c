/*
** http_server.c -- a stream socket http_server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

//#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 4096 // max number of bytes we can get at once

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

#include <string.h>
#include <stdlib.h>

char* parse(char* request) {
    char* dir = NULL;
    int len = strlen(request);
    int i = 0;

    while (i < len) {
        if (request[i] == '/') {
            int j = i + 1;
            while (request[j] != ' ' && request[j] != '\0') {
                j++;
            }
            dir = strndup(request + i, j - i);
            break;
        }
        i++;
    }

    return dir;
}


void result(char *dir, int back_fd) {
    char content[MAXDATASIZE];
    FILE *fp = fopen(dir, "r");
    if (fp == NULL) {
        char response[40] = "HTTP/1.1 404 Not Found\r\n\r\n";
        ssize_t bytes_sent = send(back_fd, response, strlen(response), 0);
        if (bytes_sent == -1) {
            perror("send");
            exit(1);
        }
        return ;
    }
    else {
        char response[50] = "HTTP/1.1 200 OK\r\n\r\n";
        ssize_t bytes_sent = send(back_fd, response, strlen(response), 0);
        if (bytes_sent == -1) {
            perror("send");
            exit(1);
        }
    }

    // read file and send
    size_t content_length;
    while (!feof(fp)) {
        size_t content_length = fread(content, 1, sizeof(content), fp);
        if (content_length <= 0) {
            printf("Finish reading and file is blank");
            ssize_t bytes_sent = send(back_fd, "bye", strlen("bye"), 0);
            if (bytes_sent == -1) {
                perror("send");
                exit(1);
            }
            break;
        } else {
            ssize_t bytes_sent = send(back_fd, content, content_length, 0);
            if (bytes_sent == -1) {
                perror("send");
                exit(1);
            }
        }
    }

    fclose(fp); // 关闭文件
    printf("Finish reading");
    return ;
}


int main(int args, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

    //get header， hostname, port,

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("http_server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("http_server: bind");
			continue;
		}
		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "http_server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("http_server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("http_server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
/*			if (send(new_fd, "Hello, world!", 13, 0) == -1)
            perror("send");*/

            char response[MAXDATASIZE];

            //while(1){
                ssize_t bytes_received = recv(new_fd, response, MAXDATASIZE - 1, 0);
                if (bytes_received == -1) {
                    perror("recv");
                    exit(1);
                }
                response[bytes_received] = '\0';


                char *dir= parse(response);
                char *new_dir;
                new_dir = (char *)malloc(strlen(dir) + 2);
                sprintf(new_dir, ".%s", dir);
                dir = new_dir;
                //printf("%s",dir);

                //char dir[100]="/Users/livlon/CLionProjects/ece438_MP1/yam.txt";

                char identify[5];
                strncpy(identify,response,3);
                identify[4] = '\0';  // 添加字符串终止符
                if(strcmp(identify, "GET") != 0){
                    char response[50] = "HTTP/1.1 400 Bad Request\r\n\r\n";
                    ssize_t bytes_sent = send(new_fd, response, strlen(response), 0);
                    if (bytes_sent == -1) {
                        perror("send");
                        exit(1);
                    }
                }
                printf("%s",response);
                result(dir,new_fd);
                break;
           //}
        }
			close(new_fd);
		}
	return 0;
}


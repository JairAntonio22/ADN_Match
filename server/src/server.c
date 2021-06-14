#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>
#include "seq.h"
#define SIZE 12
#define PORT 3000
#define SA struct sockaddr

FILE *fp;
// Function designed for chat between client and server.
void func(int sockfd)
{
    int instruction = 0;
    int num_bloques_secuencia = 0;
    char buff[SIZE];
	

	seq_t *seqs;
	seq_t genoma;

    // infinite loop for chat

	printf("Vamoa escuchar al cliente\n");
	
    for (;;) {
        if(instruction == 0){

            bzero(buff, SIZE);
            read(sockfd, buff, SIZE);
            num_bloques_secuencia = atoi(buff);
            instruction++;
        }else if(instruction == 1){
			seqs = malloc(sizeof(seq_t));
            for(int i = 0; i < num_bloques_secuencia; i++){
                bzero(buff, SIZE);
                read(sockfd, buff, SIZE);
                seqs[i].size = atoi(buff);
                seqs[i].data = malloc(seqs[i].size+1);
                //bzero(buff, seqs[i].size);
                read(sockfd, seqs[i].data, seqs[i].size+1);
            }
            instruction++;
        }else if(instruction == 2){
            seq_t seq;
            //char ];
            bzero(buff, SIZE);
            read(sockfd, buff, SIZE);
            genoma.size = atoi(buff);

            // bzero(buff, seqs[i].size);
            read(sockfd, genoma.data, genoma.size);
            strcpy(seq.data, buff);

            int pos[num_bloques_secuencia];
            float percent;

            batch_search(seqs, num_bloques_secuencia, genoma, pos, &percent);

            for (int i = 0; i < num_bloques_secuencia; i++) {
                printf("String %d is at position %d\n", i, pos[i]);
            }

            printf("Match percent: %2.2f%%\n", percent * 100);

            instruction++;
        }else{
            fprintf(fp, "Server Exit...\n");
            break;
        }
        //bzero(buff, MAX);
        fprintf(fp, "--------------------------------------------\n");
    }
}

// Driver function
int main()
{
    int sockfd, connfd;
    unsigned int len;
    struct sockaddr_in servaddr, cli;
    printf("aAAAAAAAAAAAAAAAAAAAAAAAA");

    //starting daemonize
    //printf("Starting daemonize\n");
    //daemonize();

    fp = fopen ("Log.txt", "w+");

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(fp, "socket creation failed...\n");
        exit(0);
    }
    else
        fprintf(fp, "Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
        fprintf(fp, "socket bind failed...\n");
        exit(0);
    }
    else
        fprintf(fp, "Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        fprintf(fp, "Listen failed...\n");
        exit(0);
    }
    else
        fprintf(fp, "Server listening..\n");
    len = sizeof(cli);

    while(1){
    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA *)&cli, &len);
    if (connfd < 0) {
        fprintf(fp, "server acccept failed...\n");
        exit(0);
        }
    else{
            fprintf(fp, "server acccept the client...\n");
            // Function for chatting between client and server
            func(connfd);
            close(connfd);
        }
    }
    // /fclose(fp);
    // syslog (LOG_NOTICE, "First daemon terminated.");
    // closelog();/
    return EXIT_SUCCESS;
}
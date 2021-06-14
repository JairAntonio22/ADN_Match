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
    int num_map =0;
	

	seq_t *seqs;
	seq_t genoma;

    // infinite loop for chat
	
    for (;;) {
        if(instruction == 0){
			printf("Leyendo numero de bloques\n");
            bzero(buff, SIZE);
            read(sockfd, buff, SIZE);
            num_bloques_secuencia = atoi(buff);
			printf("Size de bloques: %i\n", num_bloques_secuencia);
            instruction++;

        }else if(instruction == 1){
			printf("Leyendo secuencias\n");
			seqs = malloc(sizeof(seq_t) * num_bloques_secuencia) ;
            for(int i = 0; i < num_bloques_secuencia; i++){
                bzero(buff, SIZE);
                read(sockfd, buff, SIZE);
                seqs[i].size = atoi(buff);
                seqs[i].data = malloc(seqs[i].size+1);
                read(sockfd, seqs[i].data, seqs[i].size+1);
				printf("Size: %i Data: %s\n", seqs[i].size, seqs[i].data);
            }
            instruction++;
        }else if(instruction == 2){
			printf("Leyendo genoma\n");
            bzero(buff, SIZE);
            read(sockfd, buff, SIZE);
            genoma.size = atoi(buff);

			genoma.data = malloc(sizeof(char) * genoma.size + 1);
            read(sockfd, genoma.data, genoma.size + 1);
			printf("Size Genoma: %i Data Genoma: %s\n", genoma.size, genoma.data);

			if (genoma.data != NULL) {
				int pos[num_bloques_secuencia];
				float percent;

				batch_search(seqs, num_bloques_secuencia, genoma, pos, &percent);

				for (int i = 0; i < num_bloques_secuencia; i++) {
					printf("String %s is at position %d\n", seqs[i].data, pos[i]);
				}

				printf("Match percent: %2.2f%%\n", percent * 100);
                //copiar porcentaje al buffer y mandar
                sprintf(buff, "%2.2f", percent*100);
                send(sockfd, buff, SIZE, percent*100);

                //copiar secuencias mapeadas y mandar
                sprintf(buff, "%d", num_bloques_secuencia);
                send(sockfd, buff, SIZE, num_bloques_secuencia);

                //copiar secuencias no mapeadas y mandar 
                //sprintf(buff, "%d", );
                //send(sockfd, );

			} else {
				printf("Genoma no recibido\n");
			}

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

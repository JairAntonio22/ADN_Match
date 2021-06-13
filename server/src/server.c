#include "seq.h"
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
#define MAX 80
#define PORT 3000
#define SA struct sockaddr
FILE *fp= NULL;

// Daemonize

static void daemonize()
{
    pid_t pid;
    
    /* Fork off the parent process */
    pid = fork();
    
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    
     /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);
    
    /* Catch, ignore and handle signals */
    /*TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    /* Fork off for the second time*/
    pid = fork();
    
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    
    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    /* Set new file permissions */
    umask(027);
    
    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    //chdir("/Users/Hector/progrAva");
    
    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }
    
    /* Open the log file */
    openlog ("firstdaemon", LOG_PID, LOG_DAEMON);
}



// Function designed for chat between client and server.
void func(int sockfd)
{
	char buff[MAX];
	// infinite loop for chat
	for (;;) {
		bzero(buff, MAX);
		// read the message from client and copy it in buffer
		read(sockfd, buff, sizeof(buff));
		// print buffer which contains the client contents
		fprintf(fp, "From client: %s\n", buff);
		if (strcmp(buff, "exit") == 0) {
			fprintf(fp, "Server Exit...\n");
			break;
		}
		char ** str_arr = calloc(sizeof(char*), 500);

		//AQUI IRIA LA RECEPCION DE LOS MENSAJES DEL CLIENTE
		
		bzero(buff, MAX);
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
	printf("Starting daemonize\n");
    daemonize();

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
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
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
	connfd = accept(sockfd, (SA*)&cli, &len);
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
	fclose(fp);
	syslog (LOG_NOTICE, "First daemon terminated.");
    closelog();
	return EXIT_SUCCESS;
}


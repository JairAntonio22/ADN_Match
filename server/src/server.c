#include "db.h"
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
		if (strcmp(buff, "logout") == 0) {
			fprintf(fp, "Server Exit...\n");
			break;
		}
		char ** str_arr = calloc(sizeof(char*), 500);
		// Extract the first token
		char * token = strtok(buff, ",");
		// loop through the string to extract all other tokens
		int contador = 0;
		while( token != NULL ) {
			str_arr[contador] = strdup(token); //printing each token
			token = strtok(NULL, ",");
			contador++;
		}
		char * mensaje;
		//Login
		Table *tabla = load_table("Usuarios.csv");
		int existe = 0;
		if(strcmp(str_arr[0], "login") == 0){
			for (int i = 1; i < tabla->rows; i++){
				if(strcmp(tabla->data[i][0], str_arr[1]) == 0){
					if(strcmp(tabla->data[i][1], str_arr[2]) == 0){
						fprintf(fp, "Login Successful\n");
						mensaje = "success";
						write(sockfd, mensaje, strlen(mensaje));
						existe = 1;
					}
				} 
			}
			if(existe == 0){
				fprintf(fp, "Login Incorrect\n");
				mensaje = "error";
				write(sockfd, mensaje, strlen(mensaje));
			}
		}
		DB *db = load_db("MyDB.txt");
		//Insert
		if(strcmp(str_arr[0], "insert") == 0){
			char ** values = calloc(sizeof(char*), contador-2);
			for(int i = 0; i < contador-2; i++){
				values[i] = str_arr[i+2];
			}
			Error error = insert_db(db, str_arr[1], values);
			switch (error)
			{
				case SuccessOperation:{
					Error guardar = save_db(db);
					if(guardar == SuccessOperation){
						fprintf(fp, "%s se actualizo correctamente\n", str_arr[1]);
						mensaje = "Se realizo el insert de manera exitosa\n";
					}else{
						fprintf(fp, "%s no se pudo actualizar\n", str_arr[1]);
						mensaje = "Hubo un problema con al insertar los datos\n";
					}
					write(sockfd, mensaje, strlen(mensaje));
				break;
				}

				case NullPtrError:
					mensaje = "ERROR: la base de datos es nula\n";
					write(sockfd, mensaje, strlen(mensaje));
				break;

				case TableNotFound:
					mensaje = "ERROR: No se pudo encontrar la tabla ";
					write(sockfd, mensaje, strlen(mensaje));
				break;
				
				default:
					mensaje = "ERROR: La cantidad de values es menor al numero de columnas";
					write(sockfd, mensaje, strlen(mensaje));
				break;
			}
		}
		//Select All
		if(strcmp(str_arr[0], "select_all") == 0){
			Table *table = select_db(db, str_arr[1], 1, NULL, NULL, select_all);
        	if(table != NULL){
			print_table(table);
			fprintf(fp, "%s\n", encode_table(table));
			write(sockfd, encode_table(table), strlen(encode_table(table)));
			delete_table(table);
			}else{
				char * mensaje = "ERROR: Verifica bien los datos";
				write(sockfd, mensaje, strlen(mensaje));
			}
		}
		//Select All where
		if(strcmp(str_arr[0], "select_all_where") == 0){
			char *where[2];
            where[0] = strdup(str_arr[2]);
            where[1] = strdup(str_arr[3]);

			Table *table = select_db(db, str_arr[1], 0, NULL, where,select_where);
			if(table != NULL){
				print_table(table);
				fprintf(fp, "%s\n", encode_table(table));
				write(sockfd, encode_table(table), strlen(encode_table(table)));
				delete_table(table);
				free(where[0]);
				free(where[1]);
			}else{
				char * mensaje = "ERROR: Verifica bien los datos";
				write(sockfd, mensaje, strlen(mensaje));
			}
		}
		//Select all cols
		if(strcmp(str_arr[0], "select_all_cols") == 0){
			char ** cols = calloc(sizeof(char*), contador-2);
			for(int i = 0; i < contador-2; i++){
				cols[i] = str_arr[i+2];
			}
			Table *table = select_db(db, str_arr[1], contador-2,cols,NULL,select_cols);
 			if(table != NULL){
				print_table(table);
				fprintf(fp, "%s\n", encode_table(table));
				write(sockfd, encode_table(table), strlen(encode_table(table)));
				delete_table(table);
			}else{
				char * mensaje = "ERROR: Verifica bien los datos";
				write(sockfd, mensaje, strlen(mensaje));
			}
			
		}
		//Select cols where
		if(strcmp(str_arr[0], "select_cols_where") == 0){
			char ** cols = calloc(sizeof(char*), contador-4);
			for(int i = 0; i < contador-4; i++){
				cols[i] = str_arr[i+4];
			}
			char *where[2];
            where[0] = strdup(str_arr[2]);
            where[1] = strdup(str_arr[3]);
			Table *table = select_db(db, str_arr[1], contador-4,cols,where,select_cols_where);
 			if(table != NULL){
				print_table(table);
				fprintf(fp, "%s\n", encode_table(table));
				write(sockfd, encode_table(table), strlen(encode_table(table)));
				delete_table(table);
				free(where[0]);
				free(where[1]);
			}else{
				char * mensaje = "ERROR: Verifica bien los datos";
				write(sockfd, mensaje, strlen(mensaje));
			}
		}
		//Join
		if(strcmp(str_arr[0], "join") == 0){
			char *cols[] = {str_arr[3], str_arr[4]};
			Table *table = join_db(db, str_arr[1], str_arr[2], cols);
			if(table != NULL){
				print_table(table);
				fprintf(fp, "%s\n", encode_table(table));
				write(sockfd, encode_table(table), strlen(encode_table(table)));
				delete_table(table);
			}else{
				char * mensaje = "ERROR: Verifica bien los datos";
				write(sockfd, mensaje, strlen(mensaje));
			}
		}
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


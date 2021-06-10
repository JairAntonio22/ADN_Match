// #include "db.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <netinet/in.h>
// #include <netdb.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #define MAX 80
// #define PORT 3000
// #define SA struct sockaddr


// // Función Select All
// void selectAll(char* buff, char* tabla1){
// 	DB *db = load_db("MyDB.txt");

//     Table *table = select_db(db, tabla1, 1, NULL, NULL, NULL);
//     print_table(table);
// 	printf("%s\n", encode_table(table));

// }

// // Función Join
// void join(char *t1, char *t2, char* col1, char* col2){
// 	DB *db = load_db("MyDB.txt");
// 	char* cols[] = {col1, col2};
// 	Table *table = join_db(db, t1, t2, cols);
// }

// // Función para saber cuantos inputs escribió el usuario
// void split(char *buff){
// 	DB *db = load_db("MyDB.txt");
// 	char ** str_arr = calloc(sizeof(char*), 500);
// 	// Extract the first token
// 	char * token = strtok(buff, ",");
// 	// loop through the string to extract all other tokens
// 	int i = 0;
// 	while( token != NULL ) {
// 		str_arr[i] = strdup(token); //printing each token
// 		token = strtok(NULL, ",");
// 		i++;
// 	}
	
// 	for(int i = 0; str_arr[i] != NULL; i++){
// 		printf("%s\n", str_arr[i]);
// 	}

// 	if(strcmp(str_arr[0], "select_all") == 0){
// 		selectAll(buff, str_arr[1]);
// 	}

// 	if(strcmp(str_arr[0], "join") == 0){
// 		join(str_arr[1], str_arr[2], str_arr[3], str_arr[4]);
// 	}
// }


// // Function designed for chat between client and server.
// void func(int sockfd)
// {
// 	char buff[MAX];
// 	int n;
// 	// infinite loop for chat
// 	for (;;) {
// 		bzero(buff, MAX);

// 		// read the message from client and copy it in buffer
// 		read(sockfd, buff, sizeof(buff));
// 		// print buffer which contains the client contents
// 		printf("From client: %s\n", buff);

// 		char cadenaQuery[sizeof(buff)];
// 		char cadenaTabla1[sizeof(buff)];
// 		int esComa = 0, sizeQuery = 0, sizeTabla1 = 0;
		
// 		char* query = malloc(sizeQuery);
// 		char* tabla1 = malloc(sizeTabla1);
// 		strcat(query, cadenaQuery);
// 		strcat(tabla1, cadenaTabla1);
		
// 		int inicio = sizeQuery + sizeTabla1 + 1;
// 		esComa = 0;

// 		bzero(buff, MAX);
// 		n = 0;
// 		// copy server message in the buffer
// 		while ((buff[n++] = getchar()) != '\n')
// 			;

// 		// and send that buffer to client
// 		write(sockfd, buff, sizeof(buff));

// 		// if msg contains "Exit" then server exit and chat ended.
// 		if (strncmp("exit", buff, 4) == 0) {
// 			printf("Server Exit...\n");
// 			break;
// 		}
// 	}
// }

// // Driver function
// int main(){
// 	int sockfd, connfd, len;
// 	struct sockaddr_in servaddr, cli;

// 	// socket create and verification
// 	sockfd = socket(AF_INET, SOCK_STREAM, 0);
// 	if (sockfd == -1) {
// 		printf("socket creation failed...\n");
// 		exit(0);
// 	}
// 	else
// 		printf("Socket successfully created..\n");
// 	bzero(&servaddr, sizeof(servaddr));

// 	// assign IP, PORT
// 	servaddr.sin_family = AF_INET;
// 	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
// 	servaddr.sin_port = htons(PORT);

// 	// Binding newly created socket to given IP and verification
// 	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
// 		printf("socket bind failed...\n");
// 		exit(0);
// 	}
// 	else
// 		printf("Socket successfully binded..\n");

// 	// Now server is ready to listen and verification
// 	if ((listen(sockfd, 5)) != 0) {
// 		printf("Listen failed...\n");
// 		exit(0);
// 	}
// 	else
// 		printf("Server listening..\n");
// 	len = sizeof(cli);

// 	// Accept the data packet from client and verification
// 	connfd = accept(sockfd, (SA*)&cli, &len);
// 	if (connfd < 0) {
// 		printf("server acccept failed...\n");
// 		exit(0);
// 	}
// 	else
// 		printf("server acccept the client...\n");

// 	// Function for chatting between client and server
// 	func(connfd);

// 	// After chatting close the socket
// 	close(sockfd);
// }


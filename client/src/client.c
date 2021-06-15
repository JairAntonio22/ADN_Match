#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

// Server IP
#define SERVER_IP "127.0.0.1"
// HTTP port
#define PORT 3000

#define OK_INIT 0
#define OK_READ_REFERENCE 0
#define OK_READ_SAMPLE 0
#define OK_UPLOAD_REFERENCE 0
#define OK_UPLOAD_SAMPLE 0
#define OK_PRINT_RESULTS 0
#define OK_FINISH 0
#define ERROR_SOCKET_NOT_CREATED 1
#define ERROR_SERVER_NOT_CONNECTED 2
#define ERROR_REQUEST_NOT_SENT 3
#define ERROR_REPLY_NOT_RECEIVED 4

#define INT_LENGTH 12

// Buffer sizes
#define INPUT_BUFFER_SIZE 64

typedef enum {false, true} bool;

int init();
int upload_reference();
int upload_sample();
int print_results();
int finish();

// Server socket descriptor
int socket_desc;
// Server info struct
struct sockaddr_in server;

// Request buffer to send data to server
char request_buffer[INT_LENGTH];

// Global flag to check connection status
bool connected = false;

int main()
{
    // //1. Init: create socket and connect to server
    // switch (init())
    // {
    // case OK_INIT:
    //     printf("Connected\n");
    //     break;
    // case ERROR_MEMORY_ALLOCATION_FAILED:
    //     printf("Memory allocation failed\n");
    //     return 1;
    //     break;
    // case ERROR_SOCKET_NOT_CREATED:
    //     printf("Could not create socket\n");
    //     return 1;
    //     break;
    // case ERROR_SERVER_NOT_CONNECTED:
    //     printf("Could not connect to server\n");
    //     return 1;
    //     break;
    // default:
    //     break;
    // }

    init();

    upload_reference();
    upload_sample();    

    print_results();

    finish();
}

int init()
{
    // 1. Create new socket using IPv4 and TCP/IP
    if ((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return ERROR_SOCKET_NOT_CREATED;

    // 2. Define server values: address family, address and port
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 3. Connect socket to server
    if (connect(socket_desc, (struct sockaddr*) &server, sizeof(server)) < 0)
        return ERROR_SERVER_NOT_CONNECTED;

    // 4. Connection successful
    connected = true;
    return OK_INIT;
}

int upload_reference()
{
    // Input buffer
    char input_buffer[INPUT_BUFFER_SIZE];

    // Read reference sequence file name from console
    printf("Ingresa nombre de archivo con secuencia de referencia: ");
    fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
    input_buffer[strlen(input_buffer) - 1] = '\0';

    if (send(socket_desc, input_buffer, INPUT_BUFFER_SIZE, 0) < 0)
        return ERROR_REQUEST_NOT_SENT;
}

int upload_sequence()
{
    // Input buffer
    char input_buffer[INPUT_BUFFER_SIZE];

    // Read sample sequence file name from console
    printf("Ingresa nombre de archivo con secuencia muestra: ");
    fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
    input_buffer[strlen(input_buffer) - 1] = '\0';

    if (send(socket_desc, input_buffer, INPUT_BUFFER_SIZE, 0) < 0)
        return ERROR_REQUEST_NOT_SENT;
}

int print_results()
{
    char percentage[INT_LENGTH];
    char mapped_sequences[INT_LENGTH];
    char unmapped_sequences[INT_LENGTH];
    char match_position[INT_LENGTH];
    char block_count[INT_LENGTH];

    //Receive percentage
    if (recv(socket_desc, percentage, INT_LENGTH, 0) < 0) {
		printf("error 1\n");
        return ERROR_REPLY_NOT_RECEIVED;
	}

    // Receive number of mapped sequences
    if (recv(socket_desc, mapped_sequences, INT_LENGTH, 0) < 0) {
		printf("error 2\n");
        return ERROR_REPLY_NOT_RECEIVED;
	}

    // Receive number of unmapped sequences
    if (recv(socket_desc, unmapped_sequences, INT_LENGTH, 0) < 0) {
		printf("error 3\n");
        return ERROR_REPLY_NOT_RECEIVED;
	}

    // Receive number of blocks
    if (recv(socket_desc, block_count, INT_LENGTH, 0) < 0) {
		printf("error 4\n");
        return ERROR_REPLY_NOT_RECEIVED;
	}

    // Reference block count
    int count = atoi(block_count);

    // Receive and print results per block
    for (int i = 0; i < count; i++)
    {
        if (recv(socket_desc, match_position, INT_LENGTH, 0) < 0)
            return ERROR_REPLY_NOT_RECEIVED;

        if(strcmp(match_position, "-1") == 0)
            printf("Bloque %i no se encontro\n", (i + 1));
        else
            printf("Bloque %i a partir del caracter %s\n", (i + 1), match_position);
    }    

    printf("\n");
    printf("El archivo cubre el %s%% del genoma de referencia\n", percentage);
    printf("%s secuencias mapeadas\n", mapped_sequences);
    printf("%s secuencias no mapeadas\n", unmapped_sequences);

    return OK_PRINT_RESULTS;
}

int finish()
{
    close(socket_desc);

    return OK_FINISH;
}
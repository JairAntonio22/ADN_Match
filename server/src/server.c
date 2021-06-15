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
#define PORT 3000
#define OK_INIT 0
#define OK_READ_REFERENCE 0
#define OK_READ_SAMPLE 0
#define OK_UPLOAD_REFERENCE 0
#define OK_UPLOAD_SAMPLE 0
#define OK_PRINT_RESULTS 0
#define OK_FINISH 0
#define ERROR_MEMORY_ALLOCATION_FAILED 1
#define ERROR_SOCKET_NOT_CREATED 2
#define ERROR_SERVER_NOT_CONNECTED 3
#define ERROR_FILE_NOT_EXISTS 4
#define ERROR_FILE_OTHER 5
#define ERROR_NON_NUCLEOBASE_FOUND 6
#define ERROR_REQUEST_NOT_SENT 7
#define ERROR_REPLY_NOT_RECEIVED 8

#define KB 1024
#define MB 1048576
#define INT_LENGTH 12

// Buffer sizes
// Menu selection size
#define SELECTION_BUFFER_SIZE 1
// Reference sequence size
#define BLOCK_BUFFER_BASE_SIZE (20 * KB)
// Sample sequence size
#define SAMPLE_BUFFER_BASE_SIZE (20 * MB)
// Input buffer size for reference and sample sequences file names
#define INPUT_BUFFER_SIZE 64

// Request size
#define REQUEST_BUFFER_SIZE 513
// Server reply size
#define REPLY_BUFFER_SIZE 2001

// Deault variable values
#define INITIAL_BLOCK_COUNT 1
#define INITIAL_BLOCK_SIZE 0
#define INITIAL_BLOCK_SIZE_FACTOR 1

typedef enum {false, true} bool;

int socket_desc;
// Server info struct

// Input buffer
char input_buffer[INPUT_BUFFER_SIZE];
// Request buffer to send data to server
char request_buffer[INT_LENGTH];
// Reply buffer to receive data from server
char reply_buffer[REPLY_BUFFER_SIZE];

// Reference sequence buffer
char* reference_buffer;
// Sample sequence buffer
char* sample_buffer;
// Nucleobase character
char nucleobase;
// Reference block count
int block_count = INITIAL_BLOCK_COUNT;

int max_length = 0;
//
char percentage[INT_LENGTH];
char mapped_sequences[INT_LENGTH];
char unmapped_sequences[INT_LENGTH];
char match_position[INT_LENGTH];

// Reference file
FILE* f_reference = NULL;
// Sequence file
FILE* f_sample = NULL;
//

bool is_nucleobase(char c)
{
    if(c == 'A' || c == 'T' || c == 'C' || c == 'G')
        return true;

    return false;
}

seq_t* read_reference(char *filename) {
    printf("Filename: %s", filename);

    // Open file with given name
    FILE *f_reference = fopen(filename, "r");

    // Initialize variables
    int block_size_factor = INITIAL_BLOCK_SIZE_FACTOR;
    int limit_block_size = block_size_factor * BLOCK_BUFFER_BASE_SIZE;
    int current_block_size = INITIAL_BLOCK_SIZE;
    
    // Create first block    
    seq_t *sequence_blocks = malloc(sizeof(seq_t));
    sequence_blocks[block_count - 1].data = malloc(limit_block_size);

	char nucleobase;

    // Read file character by character
    while ((nucleobase = getc(f_reference)) != EOF)
    {   
        // Allocate more memory if there is not enough
        if (current_block_size == limit_block_size)
        {
            limit_block_size = ++block_size_factor * BLOCK_BUFFER_BASE_SIZE;
            sequence_blocks[block_count - 1].data = realloc(sequence_blocks[block_count - 1].data, limit_block_size);

        }

        // Check if character is a nucleobase

        // End of block found
        
        //if (isspace(nucleobase))
        if (nucleobase == '\r')
        {
            if ((nucleobase = getc(f_reference)) == '\n')
            {
                // Save current block size
                sequence_blocks[block_count - 1].size = current_block_size;

                if (current_block_size > max_length)
                    max_length = current_block_size;

                // Delimit string with termination char
                sequence_blocks[block_count - 1].data[current_block_size] = '\0';

                // Reset block variables
                current_block_size = INITIAL_BLOCK_SIZE;
                block_size_factor = INITIAL_BLOCK_SIZE_FACTOR;
                limit_block_size = block_size_factor * BLOCK_BUFFER_BASE_SIZE;

                // Create new block
                block_count++;
                printf("Block count: %i\n", block_count);
                sequence_blocks = realloc(sequence_blocks, block_count * sizeof(seq_t));
                sequence_blocks[block_count - 1].data = malloc(limit_block_size);
            }
        }
        else
        {
            // Store character
            sequence_blocks[block_count - 1].data[current_block_size] = nucleobase;
            // Update current block size
            current_block_size++;
        }
    }

    //printf("%s tam %d\n", sequence_blocks[0].data, sequence_blocks[0].size);
    block_count--;

    return sequence_blocks;

}

seq_t* read_sample(char *filename) {
    // Check if file with given name exists
    f_sample = fopen(filename, "r");

    // Initialize variables
    int block_size_factor = INITIAL_BLOCK_SIZE_FACTOR;
    int limit_block_size = block_size_factor * BLOCK_BUFFER_BASE_SIZE;
    int current_block_size = INITIAL_BLOCK_SIZE;

    seq_t *sample_block = malloc(sizeof(seq_t));
    sample_block->data = malloc(limit_block_size);

    while ((nucleobase = getc(f_sample)) != EOF)
    {
        // Allocate more memory if there is not enough
        if (current_block_size == limit_block_size)
        {
            limit_block_size = ++block_size_factor * SAMPLE_BUFFER_BASE_SIZE;
            sample_block->data = realloc(sample_block->data, limit_block_size);
        }

        // Check if character is a nucleobase

        if (nucleobase == '\r')
            if ((nucleobase = getc(f_sample)) == '\n')
                break;

        // Store character
        sample_block->data[current_block_size] = nucleobase;
        // Update current block size
        current_block_size++;
    }

    current_block_size--;

    // Save sample size
    sample_block->size = current_block_size;

    // Delimit string with termination char
    sample_block->data[current_block_size] = '\0';

    return sample_block;
}
FILE *fp;
// Function designed for chat between client and server.
void func(int sockfd)
{
    //int instruction = 0;
    //int num_bloques_secuencia = 0;
    char buff[80];
    int num_map = 0;
    float percent;


	/*seq_t *seqs;
	seq_t genoma;*/

    // infinite loop for chat
    for (;;) {
        //Leer sample
       // printf("Leyendo sample\n");
        bzero(buff, 80);
        read(sockfd, buff, 80);
        seq_t *seqs = read_reference(buff);

        printf("y a se leyo seqs\n");

        //Leer sequences
        //printf("Leyendo sequences\n");
        bzero(buff, 80);
        read(sockfd, buff, 80);
        seq_t *seq = read_sample(buff);
        
        printf("ya se leyo seq\n");


        int pos[block_count];
        printf("seqs: %d, block count: %d, seq: %d, nummap:%d\n",seqs[0].size, block_count, seq[0].size, num_map);
        batch_search(seqs, block_count, *seq, pos, &percent, &num_map);
        printf("se acaba de hacer el batch search aaaaaaaaaaaaaaaaa");
        printf("seqs: %d, block count: %d, seq: %d, percent: %f, nummap:%d\n", seqs[0].size, block_count, seq[0].size, percent, num_map);
       	// for (int i = 0; i < num_bloques_secuencia; i++) {
        // 	printf("String  is at position %d\n", pos[i]);
        // }
        
        printf("Match percent: %2.2f%%\n", percent * 100);
        //copiar porcentaje al buffer y mandar
        sprintf(buff, "%2.2f", percent*100);
        send(sockfd, buff, SIZE, 0);

        //copiar secuencias mapeadas y mandar
        sprintf(buff, "%d", num_map);
        send(sockfd, buff, SIZE, 0);

        //copiar secuencias no mapeadas y mandar 
        sprintf(buff, "%d", block_count-num_map);
        send(sockfd, buff, SIZE, 0);

        //copiar y mandar las posiciones de las secuencias
        for(int i = 0; i < block_count; i++){
            sprintf(buff, "%d", pos[i]);
            send(sockfd, buff, SIZE, 0);
        }
        /*if(instruction == 0){
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
                //Leer tamaño del bloque
                read(sockfd, buff, SIZE);
                seqs[i].size = atoi(buff);
                //Leer numero de paquetes
                read(sockfd, buff, SIZE);
                seqs[i].numero_paquetes = atoi(buff);
            }
            

            //printf("%s tam %d\n", seqs[0].data, seqs[0].size);
            instruction++;
        }else if(instruction == 3){
			printf("Leyendo genoma\n");
            bzero(buff, SIZE);
            read(sockfd, buff, SIZE);
            genoma.size = atoi(buff);

			genoma.data = malloc(sizeof(char) * genoma.size + 1);
            read(sockfd, genoma.data, genoma.size + 1);
			printf("Size Genoma: %i Data Genoma: \n", genoma.size);

			if (genoma.data != NULL) {
				int pos[num_bloques_secuencia];
				float percent;

				batch_search(seqs, num_bloques_secuencia, genoma, pos, &percent, &num_map);

				// for (int i = 0; i < num_bloques_secuencia; i++) {
				// 	printf("String  is at position %d\n", pos[i]);
				// }

				printf("Match percent: %2.2f%%\n", percent * 100);
                //copiar porcentaje al buffer y mandar
                sprintf(buff, "%2.2f", percent*100);
                send(sockfd, buff, SIZE, 0);

                //copiar secuencias mapeadas y mandar
                sprintf(buff, "%d", num_map);
                send(sockfd, buff, SIZE, 0);

                //copiar secuencias no mapeadas y mandar 
                sprintf(buff, "%d", num_bloques_secuencia-num_map);
                send(sockfd, buff, SIZE, 0);

                //copiar y mandar las posiciones de las secuencias
                for(int i = 0; i < num_bloques_secuencia; i++){
                    sprintf(buff, "%d", pos[i]);
                    send(sockfd, buff, SIZE, 0);
                }

			} else {
				printf("Genoma no recibido\n");
			}

            instruction++;
        }else{
            fprintf(fp, "Server Exit...\n");
            break;
        }*/
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

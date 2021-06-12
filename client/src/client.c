#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "seq.h"

// Server IP
#define SERVER_IP "127.0.0.1"
// HTTP port
#define PORT 3000

#define OK_INIT 0
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

#define MB 1048576

// Buffer sizes
// Menu selection size
#define SELECTION_BUFFER_SIZE 1
// Reference sequence size
//#define BLOCK_BUFFER_BASE_SIZE (2 * MB)
#define BLOCK_BUFFER_BASE_SIZE (1 * MB)
// Sample sequence size
#define SAMPLE_BUFFER_BASE_SIZE (1 * MB)
// Input buffer size for reference and sample sequences file names
#define INPUT_BUFFER_SIZE 64

// Request size
#define REQUEST_BUFFER_SIZE 513
// Server reply size
#define REPLY_BUFFER_SIZE 2001

#define UPLOAD_REFERENCE 1
#define UPLOAD_SEQUENCE 2
#define QUIT 3

// Deault variable values
#define INITIAL_BLOCK_COUNT 1
#define INITIAL_BLOCK_SIZE 0
#define INITIAL_BLOCK_SIZE_FACTOR 1

typedef enum {false, true} bool;

int init();
int upload_reference();
int upload_sample();
int print_results();
int send_size();
int finish();

bool is_nucleobase(char c);

// Server socket descriptor
int socket_desc;
// Server info struct
struct sockaddr_in server;

// Input buffer
char input_buffer[INPUT_BUFFER_SIZE];
// Reply buffer to receive data from server
char reply_buffer[REPLY_BUFFER_SIZE];
// Request buffer to send data to server
char request_buffer[REQUEST_BUFFER_SIZE];

// Reference sequence buffer
char* reference_buffer;
// Sample sequence buffer
char* sample_buffer;
// Nucleobase character
char nucleobase;

// Global flag to check connection status
bool connected = false;
// Global flags to check reference and sample sequences status
int reference_status, sample_status;
// Dinamically allocated memory size factor
int block_size_factor;
// Dinamically allocated memory sizes
long unsigned int limit_block_size;
// Reference and sample block lengths
long unsigned int current_block_size;
// Reference block count
int block_count;

// Reference file
FILE* f_reference = NULL;
// Sequence file
FILE* f_sample = NULL;
// 
seq_t* sequence_blocks = NULL;
seq_t* sample_block = NULL;

int main()
{
    //1. Init: create socket and connect to server
    switch (init())
    {
    case OK_INIT:
        printf("Connected\n");
        break;
    case ERROR_MEMORY_ALLOCATION_FAILED:
        printf("Memory allocation failed\n");
        return 1;
        break;
    case ERROR_SOCKET_NOT_CREATED:
        printf("Could not create socket\n");
        return 1;
        break;
    case ERROR_SERVER_NOT_CONNECTED:
        printf("Could not connect to server\n");
        return 1;
        break;
    default:
        break;
    }

    do
    {
        // Read reference sequence file name from console
        printf("Ingresa nombre de archivo con secuencia de referencia: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        input_buffer[strlen(input_buffer) - 1] = '\0';

        // Upload reference
        switch (reference_status = upload_reference())
        {
        case ERROR_FILE_NOT_EXISTS:
            printf("El archivo no existe, intenta de nuevo\n");
            break;
        case ERROR_FILE_OTHER:
            printf("Error desconocido al abrir el archivo\n");
            break;
        case ERROR_MEMORY_ALLOCATION_FAILED:
            printf("No se pudo reservar mas memoria para almacenar la secuencia\n");
            break;
        case ERROR_NON_NUCLEOBASE_FOUND:
            printf("La secuencia contiene un caracter que no corresponde a una base nitrogenada: (%lu,%i)\n", current_block_size, nucleobase);
            break;
        default:
            break;
        }
    } while (reference_status != OK_UPLOAD_REFERENCE);

    do
    {
        // Read sample sequence file name from console
        printf("Ingresa nombre de archivo con secuencia muestra: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        input_buffer[strlen(input_buffer) - 1] = '\0';

        switch (sample_status = upload_sample())
        {
        case ERROR_FILE_NOT_EXISTS:
            printf("El archivo no existe, intenta de nuevo\n");
            break;
        default:
            break;
        }
    } while (upload_sample() != OK_UPLOAD_SAMPLE);

    switch (print_results())
    {
    case OK_PRINT_RESULTS:
        break;
    default:
        break;
    }

    finish();
}

int init()
{
    // sample_buffer_size = sample_size_factor * SAMPLE_BUFFER_BASE_SIZE;
    
    // reference_buffer = malloc(limit_block_size);
    // sample_buffer = malloc(sample_buffer_size);

    // if (reference_buffer == NULL || sample_buffer == NULL)
    //     return ERROR_MEMORY_ALLOCATION_FAILED;

    // // 1. Create new socket using IPv4 and TCP/IP
    // if ((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    //     return ERROR_SOCKET_NOT_CREATED;

    // // 2. Define server values: address family, address and port
    // server.sin_family = AF_INET;
    // server.sin_port = htons(PORT);
    // server.sin_addr.s_addr = inet_addr(SERVER_IP);

    // // 3. Connect socket to server
    // if (connect(socket_desc, (struct sockaddr*) &server, sizeof(server)) < 0)
    //     return ERROR_SERVER_NOT_CONNECTED;

    // 4. Connection successful
    connected = true;
    return OK_INIT;
}

int upload_reference()
{
    // Check if file with given name exists
    if (access(input_buffer, F_OK) != 0)
        return ERROR_FILE_NOT_EXISTS;

    // Open file with given name
    f_reference = fopen(input_buffer, "r");

    if (f_reference == NULL)
        return ERROR_FILE_OTHER;

    // Initialize variables
    block_count = INITIAL_BLOCK_COUNT;
    block_size_factor = INITIAL_BLOCK_SIZE_FACTOR;
    limit_block_size = block_size_factor * BLOCK_BUFFER_BASE_SIZE;
    current_block_size = INITIAL_BLOCK_SIZE;
    
    // Create first block    
    sequence_blocks = malloc(sizeof(seq_t));
    sequence_blocks[block_count - 1].data = malloc(limit_block_size);

    // Read file character by character
    while ((nucleobase = getc(f_reference)) != EOF)
    {   
        // Allocate more memory if there is not enough
        if (current_block_size == limit_block_size)
        {
            limit_block_size = ++block_size_factor * BLOCK_BUFFER_BASE_SIZE;
            sequence_blocks[block_count - 1].data = realloc(sequence_blocks[block_count - 1].data, limit_block_size);

            if (sequence_blocks[block_count - 1].data == NULL)
                return ERROR_MEMORY_ALLOCATION_FAILED;
        }

        // Check if character is a nucleobase
        if (is_nucleobase(nucleobase) == false && nucleobase != '\r' && nucleobase != '\n')
            return ERROR_NON_NUCLEOBASE_FOUND;

        // End of block found
        if (nucleobase == '\r')
        {
            if ((nucleobase = getc(f_reference)) == '\n')
            {
                // Save current block size
                sequence_blocks[block_count - 1].size = current_block_size;

                // Reset block variables
                current_block_size = INITIAL_BLOCK_SIZE;
                block_size_factor = INITIAL_BLOCK_SIZE_FACTOR;
                limit_block_size = block_size_factor * BLOCK_BUFFER_BASE_SIZE;

                // Create new block
                block_count++;
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

    block_count--;

    return OK_UPLOAD_REFERENCE;
}

int upload_sample()
{
    // Check if file with given name exists
    if (access(input_buffer, F_OK) != 0)
        return ERROR_FILE_NOT_EXISTS;

    f_sample = fopen(input_buffer, "r");
    
    block_size_factor = INITIAL_BLOCK_SIZE_FACTOR;
    limit_block_size = block_size_factor * BLOCK_BUFFER_BASE_SIZE;
    current_block_size = INITIAL_BLOCK_SIZE;

    sample_block = malloc(sizeof(seq_t));
    sample_block->data = malloc(limit_block_size);

    while ((nucleobase = getc(f_sample)) != EOF)
    {
        // Allocate more memory if there is not enough
        if (current_block_size == limit_block_size)
        {
            limit_block_size = ++block_size_factor * BLOCK_BUFFER_BASE_SIZE;
            sequence_blocks->data = realloc(sequence_blocks->data, limit_block_size);

            if (sequence_blocks->data == NULL)
                return ERROR_MEMORY_ALLOCATION_FAILED;
        }

        // Check if character is a nucleobase
        if (is_nucleobase(nucleobase) == false && nucleobase != '\r' && nucleobase != '\n')
            return ERROR_NON_NUCLEOBASE_FOUND;

        // Store character
        sequence_blocks->data[current_block_size] = nucleobase;
        // Update current block size
        current_block_size++;
    }

    sample_block->size = current_block_size;

    //printf("Sample size: %lu\n", current_block_size);
    
    return OK_UPLOAD_SAMPLE;
}

int print_results()
{
    return OK_PRINT_RESULTS;
}

int finish()
{
    block_count++;
    printf("Count: %i\n", block_count);

    for (int i = 0; i < block_count; i++)
        printf("Block: %i, size: %i\n", i, sequence_blocks[i].size);
    
    printf("Sample size: %i\n", sample_block->size);

    // for (int i = 0; i < block_count; i++)
    // {
    //     printf("|hola");
    //     for (int j = 0; j < sequence_blocks[i].size; j++)
    //     {
    //         if(!is_nucleobase(sequence_blocks[i].data[j]))
    //             printf("Char: %i\n", sequence_blocks[i].data[j]);

    //         printf("%c", sequence_blocks[i].data[j]);
    //     }
    //     printf("|");
    // }
    close(socket_desc);   

    for (int i = 0; i < block_count; i++)
        free(sequence_blocks[i].data);
    
    free(sequence_blocks);

    if (f_reference != NULL)
        fclose(f_reference);

    if (f_sample != NULL)
        fclose(f_sample);

    return OK_FINISH;
}

bool is_nucleobase(char c)
{
    if(c == 'A' || c == 'T' || c == 'C' || c == 'G')
        return true;

    return false;
}

// printf("|%s|\n", input_buffer);

// printf("Length: %i\n", current_block_size);

//     for (int i = 0; i < current_block_size; i++)
//     {
//         printf("%c", reference_buffer[i]);
//     }

//     printf("\n");

//printf("Size struct: %lu Memory for struct: %lu\n", sizeof(seq_t), block_count * sizeof(seq_t));
// printf("Memory for data: %lu\n", limit_block_size);
// printf("Here\n");

//printf("Block %i, size %lu\n", block_count, current_block_size);
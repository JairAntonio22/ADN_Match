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
#include <ctype.h>
#include "seq_client.h"

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

int init();
int read_reference();
int read_sample();
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

// Global flag to check connection status
bool connected = false;
// Global flags to check reference and sample sequences status
int reference_status, sample_status;
// Dinamically allocated memory size factor
int block_size_factor;
// Dinamically allocated memory sizes
int limit_block_size;
// Reference and sample block lengths
int current_block_size;
// Reference block count
int block_count;

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
        switch (reference_status = read_reference())
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
            printf("La secuencia contiene un caracter que no corresponde a una base nitrogenada: (%i,%i)\n", current_block_size, nucleobase);
            break;
        default:
            break;
        }
    } while (reference_status != OK_READ_REFERENCE);

    do
    {
        // Read sample sequence file name from console
        printf("Ingresa nombre de archivo con secuencia muestra: ");
        fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
        input_buffer[strlen(input_buffer) - 1] = '\0';

        switch (sample_status = read_sample())
        {
        case ERROR_FILE_NOT_EXISTS:
            printf("El archivo no existe, intenta de nuevo\n");
            break;
        default:
            break;
        }
    } while (sample_status != OK_READ_SAMPLE);

    // Upload reference blocks
    do
    {
        switch (reference_status = upload_reference())
        {
        case ERROR_REQUEST_NOT_SENT:
            break;
        default:
            break;
        }
    } while (reference_status != OK_UPLOAD_REFERENCE);

    // Upload sample block
    do
    {
        switch (sample_status = upload_sample())
        {
        case ERROR_REQUEST_NOT_SENT:
            break;
        default:
            break;
        }
    } while (sample_status != OK_UPLOAD_SAMPLE);

    // Receive and print results from server
    switch (print_results())
    {
    case OK_PRINT_RESULTS:
        break;
    case ERROR_REPLY_NOT_RECEIVED:
    default:
        break;
    }

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

int read_reference()
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

    return OK_READ_REFERENCE;
}

int read_sample()
{
    // Check if file with given name exists
    if (access(input_buffer, F_OK) != 0)
        return ERROR_FILE_NOT_EXISTS;

    f_sample = fopen(input_buffer, "r");
    
    block_size_factor = INITIAL_BLOCK_SIZE_FACTOR;
    limit_block_size = block_size_factor * SAMPLE_BUFFER_BASE_SIZE;
    current_block_size = INITIAL_BLOCK_SIZE;

    sample_block = malloc(sizeof(seq_t));
    sample_block->data = malloc(limit_block_size);

    while ((nucleobase = getc(f_sample)) != EOF)
    {
        // Allocate more memory if there is not enough
        if (current_block_size == limit_block_size)
        {
            limit_block_size = ++block_size_factor * SAMPLE_BUFFER_BASE_SIZE;
            sample_block->data = realloc(sample_block->data, limit_block_size);

            if (sample_block->data == NULL)
                return ERROR_MEMORY_ALLOCATION_FAILED;
        }

        // Check if character is a nucleobase
        if (is_nucleobase(nucleobase) == false && nucleobase != '\r' && nucleobase != '\n')
            return ERROR_NON_NUCLEOBASE_FOUND;

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
    
    return OK_READ_SAMPLE;
}

int upload_reference()
{    
    // Copy number of blocks to request buffer
    sprintf(request_buffer, "%i", block_count);
    
    // Send number of blocks to server
    if (send(socket_desc, request_buffer, INT_LENGTH, 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    // Send all sequence blocks: size, then data
    for (int i = 0; i < block_count; i++)
    {
        // Copy block size
        sprintf(request_buffer, "%i", sequence_blocks[i].size);

        // Send block size
        if (send(socket_desc, request_buffer, INT_LENGTH, 0) < 0)
            return ERROR_REQUEST_NOT_SENT;

        // Send block data
        if (send(socket_desc, sequence_blocks[i].data, sequence_blocks[i].size + 1, 0) < 0)
            return ERROR_REQUEST_NOT_SENT;
    }

    return OK_UPLOAD_REFERENCE;
}

int upload_sample()
{
    // Copy block size
    sprintf(request_buffer, "%i", sample_block->size);

    //Send block size
    if (send(socket_desc, request_buffer, INT_LENGTH, 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    // Send block data
    if (send(socket_desc, sample_block->data, sample_block->size + 1, 0) < 0)
        return ERROR_REQUEST_NOT_SENT;

    return OK_UPLOAD_SAMPLE;
}

int print_results()
{
    //Receive percentage
    if (recv(socket_desc, percentage, INT_LENGTH, 0) < 0)
        return ERROR_REPLY_NOT_RECEIVED;

    // Receive number of mapped sequences
    if (recv(socket_desc, mapped_sequences, INT_LENGTH, 0) < 0)
        return ERROR_REPLY_NOT_RECEIVED;

    // Receive number of unmapped sequences
    if (recv(socket_desc, unmapped_sequences, INT_LENGTH, 0) < 0)
        return ERROR_REPLY_NOT_RECEIVED;

    // Receive and print results per block
    for (int i = 0; i < block_count; i++)
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
    block_count++;

    close(socket_desc);

    for (int i = 0; i < block_count; i++)
        free(sequence_blocks[i].data);
          
    free(sequence_blocks);

    free(sample_block->data);
    free(sample_block);

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seq.h"

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

bool is_nucleobase(char c)
{
    if(c == 'A' || c == 'T' || c == 'C' || c == 'G')
        return true;

    return false;
}

seq_t* read_reference(char *filename) {
    // Open file with given name
    FILE *f_reference = fopen(filename, "r");

    // Initialize variables
    int block_count = INITIAL_BLOCK_COUNT;
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

    block_size_factor = INITIAL_BLOCK_SIZE_FACTOR;
    limit_block_size = block_size_factor * SAMPLE_BUFFER_BASE_SIZE;
    current_block_size = INITIAL_BLOCK_SIZE;

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

int main(void) {
	seq_t *seq = read_sample("S._cerevisiae_processed.txt");

	int n = 1000;

	seq_t *seqs = read_reference("s_cerevisia-1.seq");

	int pos[n];
	float percent;
	int num_map = 0;

	batch_search(seqs, n, *seq, pos, &percent, &num_map);

	for (int i = 0; i < n; i++) {
		printf("String %d is at position %10d, %10d\n, length:", i, pos[i], seq[i].size);
	}

	printf("Match percent: %2.2f%%\n", percent * 100);
	printf("Mapped sequences: %d\n", num_map);
	printf("Unmapped sequences: %d\n", n - num_map);
}

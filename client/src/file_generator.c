#include <stdio.h>
#include <stdlib.h>

int main()
{
    FILE* sample;
    FILE* sequence;

    sample = fopen("test_sample.txt", "w");
    sequence = fopen("test_sequence.txt", "w");

    fprintf(sample, "ATCGATCGATCGATCG\r\n");

    fprintf(sequence, "ATCGATCG\r\n");
    fprintf(sequence, "ATCGATC\r\n");
    fprintf(sequence, "ATCGAT\r\n");
    fprintf(sequence, "ATCGA\r\n");
    fprintf(sequence, "ATCG\r\n");

    fclose(sample);
    fclose(sequence);
}
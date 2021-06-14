#include <stdio.h>
#include <string.h>

#include "seq.h"

int main(void) {
	seq_t seq = {
		.size = 368,
		.data = "ACAAGATGCCATTGTCCCCCGGCCTCCTGCTGCTGCTGCTCTCCGGGGCCACGGCCACCGCTGCCCTGCCCCTGGAGGGTGGCCCCACCGGCCGAGACAGCGAGCATATGCAGGAAGCGGCAGGAATAAGGAAAAGCAGCCTCCTGACTTTCCTCGCTTGGTGGTTTGAGTGGACCTCCCAGGCCAGTGCCGGGCCCCTCATAGGAGAGGAAGCTCGGGAGGTGGCCAGGCGGCAGGAAGGCGCACCCCCCCAGCAATCCGCGCGCCGGGACAGAATGCCCTGCAGGAACTTCTTCTGGAAGACCTTCTCCTCCTGCAAATAAAACCTCACCCATGAATGCTCACGCAAGTTTAATTACAGACCTGAA"
	};

	int n = 5;

	seq_t seqs[] = {
		{.size = 23, .data = "GCCTCCTGCTGCTGCTGCTCTCC"},
		{.size = 23, .data = "GGACCTCCCAGGCCAGTGCCGGG"},
		{.size =  23, .data = "AAGACCTTCTCCTCCTGCAAATA"},
		{.size =  23, .data = "TTCTTCTGGAAGACCTTCTCCTC"},
		{.size =  44, .data = "CCAGGCGGCAGGAAGGCGCACCCCCCCAGCAATCCGTGCGCCGG"},
	};

	int pos[n];
	float percent;
	int num_map = 0;

	batch_search(seqs, n, seq, pos, &percent, &num_map);

	for (int i = 0; i < n; i++) {
		printf("String %d is at position %d\n", i, pos[i]);
	}

	printf("Match percent: %2.2f%%\n", percent * 100);
	printf("Mapped sequences: %d\n", num_map);
	printf("Unmapped sequences: %d\n", n - num_map);
}

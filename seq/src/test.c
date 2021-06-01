#include <stdio.h>
#include <string.h>

#include "seq.h"

int main(void) {
	seq_t seq = {
		.size = 26,
		.data = "abcdefghijklmnopqrstuvwxyz"
	};

	int n = 4;

	seq_t seqs[] = {
		{.size = 10, .data = "abcdefghij"},
		{.size = 15, .data = "defghijklmnopqrs"},
		{.size =  5, .data = "wxyza"},
		{.size =  4, .data = "wxyz"},
	};

	int pos[] = {0, 0, 0, 0};

	batch_search(seqs, n, seq, pos, NULL);

	for (int i = 0; i < n; i++) {
		printf("String %d is at position %d\n", i, pos[i]);
	}
}

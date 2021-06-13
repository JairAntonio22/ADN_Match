#ifndef SEQ_H
#define SEQ_H

typedef struct {
	int size;
	char *data;
} seq_t;

void find_prefixes(seq_t pat, int prefixes[pat.size]);

int kmp_search(seq_t str, seq_t pat);

void batch_search(seq_t seqs[], int n, seq_t seq, int *pos, float *match);

#endif

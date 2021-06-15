#ifndef SEQ_H
#define SEQ_H

typedef struct {
	int size;
	int numero_paquetes;
	char *data;
} seq_t;

void batch_search(seq_t seqs[], int n, seq_t seq, int pos[], float *match, int *num_map);

#endif

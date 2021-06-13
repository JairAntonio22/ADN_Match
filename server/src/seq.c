#include <stdlib.h>
#include <pthread.h>

#include "seq.h"

void find_prefixes(seq_t pat, int prefixes[pat.size]) {
	prefixes[0] = 0;

	int len = 0;
	int i = 1;

	while (i < pat.size) {
		if (pat.data[i] == pat.data[len]) {
			len++;
			prefixes[i] = len;
			i++;

		} else {
			if (len != 0) {
				len = prefixes[len - 1];

			} else {
				prefixes[i] = 0;
				i++;
			}
		}
	}
}

int kmp_search(seq_t seq, seq_t pat) {
	int i = 0;
	int j = 0;

	int prefixes[pat.size];
	find_prefixes(pat, prefixes);

	while (i < seq.size) {
		if (pat.data[j] == seq.data[i]) {
			i++;
			j++;
		} 

		if (j == pat.size) {
			return i - j;

		} else if (i < seq.size && pat.data[j] != seq.data[i]) {
			if (j != 0) {
				j = prefixes[j - 1];

			} else {
				i++;
			}
		}
	}

	return -1;
}

typedef struct {
	seq_t seq;
	seq_t pat;
	int *result;
	pthread_mutex_t *mutex;
	int **buffer;
	int *size;
} search_args_t;

void* search(void *v_args) {
	search_args_t args = *((search_args_t*) v_args);
	int pos = kmp_search(args.seq, args.pat);
	*args.result = pos;

	pthread_mutex_lock(args.mutex);

	args.buffer[*args.size][0] = pos;
	args.buffer[*args.size][1] = args.pat.size;
	(*args.size)++;

	pthread_mutex_unlock(args.mutex);

	return NULL;
}

typedef struct {
	seq_t seq;
	float *result;
	pthread_mutex_t *mutex;
	int **buffer;
	int *size;
} match_args_t;

void push_heap(int value[2], int *heap[2], int *size) {
	heap[*size] = value;
	(*size)++;

	int i = *size;
	int *tmp;

	while (i > 1 && heap[i][0] > heap[i / 2][0]) {
		tmp = heap[i];
		heap[i] = heap[i / 2];
		heap[i / 2] = tmp;
		i /= 2;
	}
}

void pop_heap(int *heap[2], int *size) {
	int *tmp = heap[1];
	heap[1] = heap[*size];
	heap[*size] = tmp;
	(*size)--;

	int i = 1;

	while (2 * i < *size && heap[i][0] < heap[2 * i][0]) {
		tmp = heap[i];
		heap[i] = heap[2 * i];
		heap[2 * i] = tmp;
		i *= 2;
	}
}

void* match(void *v_args) {
	return NULL;
}

void batch_search(seq_t seqs[], int n, seq_t seq, int pos[], float *percent) {
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);

	pthread_t threads[n];
	search_args_t search_args[n];

	pthread_t thread;

	int **buffer = malloc(n * sizeof(int*));
	int size = 0;

	for (int i = 0; i < n; i++) {
		buffer[i] = malloc(2 * sizeof(int));
	}

	match_args_t match_args = {
		.seq = seq,
		.result = percent,
		.mutex = &mutex,
		.buffer = buffer,
		.size = &size
	};

	pthread_create(&thread, NULL, match, (void*) &match_args);

	for (int i = 0; i < n; i++) {
		search_args[i] = (search_args_t) {
			.seq = {
				.size = seq.size,
				.data = seq.data
			},
			.pat = seqs[i],
			.result = &pos[i],
			.mutex = &mutex,
			.buffer = buffer,
			.size = &size
		};

		pthread_create(&threads[i], NULL, search, (void*) &search_args[i]);
	}

	for (int i = 0; i < n; i++) {
		pthread_join(threads[i], NULL);
	}

	pthread_join(thread, NULL);
}

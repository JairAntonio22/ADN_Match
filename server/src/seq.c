#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/sem.h>

#include <fcntl.h>

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
	int pos;
	int len;
} pair_t;

typedef struct {
	seq_t seq;
	seq_t pat;
	int *result;
	int num_results;
	int *num_map;
	pthread_mutex_t *mutex;
	sem_t *full;
	sem_t *empty;
	pair_t *buffer;
	int *size;
} search_args_t;

void* search(void *v_args) {
	search_args_t args = *((search_args_t*) v_args);

	*args.result = kmp_search(args.seq, args.pat);

	sem_wait(args.empty);
	pthread_mutex_lock(args.mutex);

	args.buffer[*args.size] = (pair_t) {
		.pos = *args.result,
		.len = args.pat.size
	};

	(*args.size)++;

	if (*args.result != -1) {
		(*args.num_map)++;
	}

	pthread_mutex_unlock(args.mutex);
	sem_post(args.full);

	return NULL;
}

typedef struct {
	seq_t seq;
	float *result;
	int num_results;
	pthread_mutex_t *mutex;
	sem_t *full;
	sem_t *empty;
	pair_t *buffer;
	int *size;
} match_args_t;

void swap(pair_t *p1, pair_t *p2) {
	pair_t tmp = *p1;
	*p1 = *p2;
	*p2 = tmp;
}

void bubble_sort(pair_t *data, int size) {
	bool sorted = true;

	for (int i = 0; i < size - 1; i++) {
		sorted = true;

		for (int j = i + 1; j < size; j++) {
			if (data[i].pos > data[j].pos) {
				swap(&data[i], &data[j]);
				sorted = false;
			}
		}

		if (sorted) {
			break;
		}
	}
}

float calculate_match(pair_t data[], int size, int total_size) {
	int match = 0;
	int max = 0;

	for (int i = 0; i < size - 1; i++) {
		if (data[i].pos >= max) {
			match += data[i].len;
			max = data[i].pos + data[i].len;

		} else if (data[i].pos + data[i].len >= max) {
			match += (data[i].pos + data[i].len) - max;
			max = data[i].pos + data[i].len;
		}
	}

	return ((float) match) / ((float) total_size);
}

void* match(void *v_args) {
	match_args_t args = *((match_args_t*) v_args);

	pair_t data[args.num_results + 1];
	int size = 0;

	int read_index = 0;

	while (read_index < args.num_results) {
		sem_wait(args.full);
		pthread_mutex_lock(args.mutex);

		pair_t entry = args.buffer[read_index];
		read_index++;

		if (entry.pos != -1) {
			data[size] = entry;
			size++;
		}

		pthread_mutex_unlock(args.mutex);
		sem_post(args.empty);
	}

	bubble_sort(data, size);

	*args.result = calculate_match(data, size, args.seq.size);

	return NULL;
}

void batch_search(seq_t seqs[], int n, seq_t seq, int pos[], float *percent, int *num_map) {
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);

    char *sem_full_name = "/my_sem_full";
	sem_unlink(sem_full_name);
	sem_t *full = sem_open(sem_full_name, O_CREAT, 0660, 0);

    if (full == SEM_FAILED) {
        perror("sem_open() failed");
        exit(EXIT_FAILURE);
    }

    char *sem_empty_name = "/my_sem_empty";
	sem_unlink(sem_empty_name);
	sem_t *empty = sem_open(sem_empty_name, O_CREAT, 0660, n);

    if (empty == SEM_FAILED) {
        perror("sem_open() failed");
        exit(EXIT_FAILURE);
    }

	pthread_t threads[n];
	search_args_t search_args[n];

	pthread_t thread;
	pair_t buffer[n];
	int size = 0;

	match_args_t match_args = {
		.seq = seq,
		.result = percent,
		.num_results = n,
		.mutex = &mutex,
		.full = full,
		.empty = empty,
		.buffer = buffer,
		.size = &size
	};

	pthread_create(&thread, NULL, match, (void*) &match_args);

	for (int i = 0; i < n; i++) {
		search_args[i] = (search_args_t) {
			.seq = seq,
			.pat = seqs[i],
			.result = &pos[i],
			.num_results = n,
			.num_map = num_map,
			.mutex = &mutex,
			.full = full,
			.empty = empty,
			.buffer = buffer,
			.size = &size
		};

		pthread_create(&threads[i], NULL, search, (void*) &search_args[i]);
	}

	for (int i = 0; i < n; i++) {
		pthread_join(threads[i], NULL);
	}

	pthread_join(thread, NULL);

	pthread_mutex_destroy(&mutex);

	sem_unlink(sem_full_name);
	sem_close(full);

	sem_unlink(sem_empty_name);
	sem_close(empty);
}
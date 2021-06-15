#ifndef SEQ_CLIENT_H
#define SEQ_CLIENT_H

typedef struct {
	int size;
	char* data;
} packet_t;

typedef struct {
	int size;
	int no_packets;
	packet_t* packets;
	int map_start;
} seq_t;
#endif

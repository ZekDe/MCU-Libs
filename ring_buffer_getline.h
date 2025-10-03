#ifndef RING_BUFFER_GETLINE_H
#define RING_BUFFER_GETLINE_H

#define RING_BUFFER_SIZE 256
#define GETLINE_BUFFER	48 // getline_isr ve türevleri içindedir

typedef struct {
    char buffer[RING_BUFFER_SIZE];
    volatile int head;
    volatile int tail;
} ring_buffer_t;

void ringBufferPut(char c);
int ringBufferGet(char *c);
int getline(void);

extern char getline_buffer[GETLINE_BUFFER];

void getline_isr(void);
//int getline_console(void);
//int getline_console_isr(void);

#endif
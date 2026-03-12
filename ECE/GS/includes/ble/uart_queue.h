#ifndef UART_QUEUE_H
#define UART_QUEUE_H

#define UART_QUEUE_MAX 64
#define UART_MSG_MAX   256
#include <stddef.h>

typedef struct
{
    char data[UART_QUEUE_MAX][UART_MSG_MAX];

    int head;
    int tail;
    int count;

} uart_queue_t;

extern uart_queue_t uart_queue;

void uart_queue_init(uart_queue_t *q);
int uart_queue_push(uart_queue_t *q, const char *msg);
int uart_queue_pop(uart_queue_t *q, char *out);
int uart_read_and_queue(int uart_fd, char *buffer, size_t size);
int ble_uart_check(int uart_fd);

#endif
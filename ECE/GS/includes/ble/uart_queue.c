#include "uart_queue.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

uart_queue_t uart_queue;

void uart_queue_init(uart_queue_t *q)
{
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

int uart_queue_push(uart_queue_t *q, const char *msg)
{
    if(q->count == UART_QUEUE_MAX)
        return -1;

    strncpy(q->data[q->tail], msg, UART_MSG_MAX-1);
    q->data[q->tail][UART_MSG_MAX-1] = '\0';

    q->tail = (q->tail + 1) % UART_QUEUE_MAX;
    q->count++;

    return 0;
}

int uart_queue_pop(uart_queue_t *q, char *out)
{
    if(q->count == 0)
        return -1;

    strcpy(out, q->data[q->head]);

    q->head = (q->head + 1) % UART_QUEUE_MAX;
    q->count--;

    return 0;
}

int uart_read_and_queue(int uart_fd, char *buffer, size_t size)
{
    int n;

    memset(buffer, 0, size);
    n = read(uart_fd, buffer, size - 1);

    if (n > 0)
    {
        buffer[n] = '\0';

        uart_queue_push(&uart_queue, buffer);

        printf("[PMOD RESPONSE]: %s\r\n", buffer);
    }

    return n;
}

int ble_uart_check(int uart_fd)
{
    char buffer[256];
    int n;

    n = read(uart_fd, buffer, sizeof(buffer) - 1);

    if (n > 0)
    {
        buffer[n] = '\0';

        uart_queue_push(&uart_queue, buffer);

        return n;
    }

    return 0;
}
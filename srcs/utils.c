#include "ft_ping.h"

#include <stdio.h>
#include <time.h>

void sleep_remaining_time(long start_time, long end_time)
{
    struct timespec ts;

    // calculate remaining time (1s - time to recv packet)
    long remaining_time_ns = 1000000000 - (start_time * 1000000 - end_time * 1000000);

    if (remaining_time_ns < 0)
        return ;
    ts.tv_sec = remaining_time_ns / 1000000000;
    ts.tv_nsec = remaining_time_ns % 1000000000;
    if (nanosleep(&ts, NULL) == -1) {
        perror("nanosleep failed");
    }
}

void clean_ping(t_ping	*ping)
{
    t_sequence *next;

    if (ping)
    {
        if (ping->socket_fd != -1)
            close(ping->socket_fd);
        if (ping->received_sequence)
        {
            t_sequence *tmp = ping->received_sequence;
            while (tmp)
            {
                next = tmp->next;
                free(tmp);
                tmp = next;
            }
        }
        free(ping);
        ping = NULL;
    }
}

void print_packet_info(int sequence, int ttl, double request_time, int status)
{
    printf("icmp_seq=%d ttl=%d time=%.1f ms",
        sequence, ttl, request_time);
    if (status == 1)
        printf(" (malformed packet)");
    else if (status == 2)
        printf(" (duplicate packet)");
    printf("\n");
}

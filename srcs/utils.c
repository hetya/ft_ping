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
    if (ping)
    {
        if (ping->socket_fd != -1)
            close(ping->socket_fd);
        free(ping);
        ping = NULL;
    }
}

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

void set_socket_options(int socket_fd)
{
    uint32_t data;
    
    // Filter out unwanted ICMP packets
    data = ~((1<<ICMP_SOURCE_QUENCH)|
    (1<<ICMP_DEST_UNREACH)|
    (1<<ICMP_TIME_EXCEEDED)|
    (1<<ICMP_PARAMETERPROB)|
    (1<<ICMP_REDIRECT)|
    (1<<ICMP_ECHOREPLY));
    if (setsockopt(socket_fd, SOL_RAW, ICMP_FILTER, (char*)&data, sizeof(data)) == -1)
    perror("WARNING: setsockopt(ICMP_FILTER)");
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &(struct timeval){1, 0}, sizeof(struct timeval)) == -1)
    perror("WARNING: setsockopt(SO_RCVTIMEO)");
}

double ft_sqrt(double num) {
    double guess = num / 2.0;
    double epsilon = 1e-3; // Precision level

    if (num < 0) {
        return (-1);
    }    
    while ((guess * guess - num) > epsilon || (num - guess * guess) > epsilon) {
        guess = (guess + num / guess) / 2.0;
    }
    
    return guess;
}

void print_statistics(t_ping *ping)
{
    double min_rtt;
    double max_rtt;
    double sum_rtt;
    double mean_rtt;
    double stddev_rtt;

    min_rtt = -1;
    max_rtt = 0;
    if (ping->nb_packets_send)
    {
        printf("\n--- %s ping statistics ---\n", ping->dest_hostname);
        printf("%d packets transmitted, %d received, %d%% packet loss\n",
            ping->nb_packets_send, ping->nb_packets_received, (ping->nb_packets_send - ping->nb_packets_received) * 100 / ping->nb_packets_send);
        for (t_sequence *tmp = ping->received_sequence; tmp; tmp = tmp->next)
        {
            if (min_rtt < 0 || tmp->request_time < min_rtt)
                min_rtt = tmp->request_time;
            if (tmp->request_time > max_rtt)
                max_rtt = tmp->request_time;
            sum_rtt += tmp->request_time;
        }
        if (min_rtt < 0)
            min_rtt = 0;
        mean_rtt = sum_rtt / (double)ping->nb_packets_received;
        stddev_rtt = 0;
        for (t_sequence *tmp = ping->received_sequence; tmp; tmp = tmp->next)
        {
            stddev_rtt += (tmp->request_time - mean_rtt) * (tmp->request_time - mean_rtt);
        }
        stddev_rtt = ft_sqrt(stddev_rtt / (double)ping->nb_packets_received);
        if (stddev_rtt < 0)
            fprintf(stderr, "Error while calculating round-trip");
        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
        min_rtt, mean_rtt, max_rtt, stddev_rtt);
    }
}

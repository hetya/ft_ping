#include "ft_ping.h"

#include <stdio.h>
#include <time.h>

void sleep_remaining_time(t_ping *ping, long start_time, long end_time)
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
    if (ping->interval_in_s > 1)
        sleep(ping->interval_in_s - 1);
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

void print_ip_packet_resume(t_ping *ping, struct iphdr *ip_header)
{
    printf("%d bytes from %s", ntohs(ip_header->tot_len) - (ip_header->ihl * 4), ping->dest_hostname);
	if (strcmp(ping->dest_hostname, ping->dest_ip))
		printf(" (%s)", ping->dest_ip);
	printf(": ");
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

void print_ip_and_icmp_details(struct iphdr *ip_header, struct icmphdr *icmp_header)
{
    printf("IP Hdr Dump:\n");
    for (int i = 0; i < 20; i++)
    {
        printf("%02x", ((unsigned char *)ip_header)[i]);
        if (i % 2)
            printf(" ");
    }
    printf("\n");
    printf("Vr %d HL %d TOS %02x Len %04x ID %04x Flg %04x off %04x TTL %02x Pro %02x cks %04x Src %s Dst %s\n",
        ip_header->version, ip_header->ihl, ip_header->tos, ntohs(ip_header->tot_len), ntohs(ip_header->id), ntohs(ip_header->frag_off), ip_header->ttl, ip_header->protocol, ntohs(ip_header->check), inet_ntoa((struct in_addr){ip_header->saddr}), inet_ntoa((struct in_addr){ip_header->daddr}));
    printf("ICMP: type %d, code %d, size %d, id 0x%x, seq 0x%x\n",
        icmp_header->type, icmp_header->code, ntohs(ip_header->tot_len) - (ip_header->ihl * 4), icmp_header->un.echo.id, icmp_header->un.echo.sequence);
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

void display_help(char *name)
{
    fprintf(stderr, "Usage: %s [-v] [-c count] [-i interval] [-q] destination\n", name);
}

int parse_args(t_ping *ping, int argc, char **argv)
{
    int opt;
    char *error_ptr;

    ping->verbose = 1;
    ping->iterations = UINT16_MAX;
    while ((opt = getopt(argc, argv, "vc:qi:")) != -1)
    {
        switch (opt) {
            case 'v': // verbose
                ping->verbose = 2;
                break;
            case 'q': // quiet
                ping->verbose = 0;
                break;
            case 'c': // number of packets to send
                long count = strtol(optarg, &error_ptr, 10);
                if (*error_ptr != '\0' || count < 0 || count > UINT16_MAX)
                {
                    fprintf(stderr, "Invalid packet count: %s\n", optarg);
                    return -1;
                }
                if (count == 0)
                    break;
                ping->iterations = count;
                break;
            case 'i': // interval between packets
                long interval = strtol(optarg, &error_ptr, 10);
                if (*error_ptr != '\0' || interval <= 0 || interval > UINT64_MAX)
                {
                    fprintf(stderr, "Invalid interval: %s\n", optarg);
                    return -1;
                }
                ping->interval_in_s = interval;
                break;
            default:
                display_help(argv[0]);
                return (-1);
        }
    }
    if (optind >= argc)
    {
        fprintf(stderr, "ping: missing host operand\n");
        fprintf(stderr, "Usage: %s [-v] [-c count] [-i interval] [-q] destination\n", argv[0]);
        return (-1);
    }
    ping->dest_hostname = argv[optind];
    return (0);
}
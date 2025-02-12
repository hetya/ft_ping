#include "ft_ping.h"

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
		printf(" (%s)", inet_ntoa((struct in_addr){ip_header->saddr}));
	printf(": ");
}

void print_packet_info(int sequence, int ttl, double request_time, int status)
{

    printf("icmp_seq=%d ttl=%d time=%.1f ms",
        sequence, ttl, request_time);
    if (status == 5)
        printf(" (malformed packet)");
    else if (status == 3)
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
    printf("Vr HL TOS Len  ID  Flg  off  TTL Pro cks  Src          Dst\n");
    printf("%d  %d  %02x %04x %04x %04x %04x %02x  %02x  %04x %s %s\n",
        ip_header->version, ip_header->ihl, ip_header->tos, ntohs(ip_header->tot_len), ntohs(ip_header->id), ntohs(ip_header->frag_off) >> 13, ntohs(ip_header->frag_off) & 0x1FFF, ip_header->ttl, ip_header->protocol, ntohs(ip_header->check), inet_ntoa((struct in_addr){ip_header->saddr}), inet_ntoa((struct in_addr){ip_header->daddr}));
    printf("ICMP: type %d, code %d, size %d, id 0x%04x, seq 0x%04x\n",
        icmp_header->type, icmp_header->code, ntohs(ip_header->tot_len) - (ip_header->ihl * 4), icmp_header->un.echo.id, icmp_header->un.echo.sequence);
}

void set_socket_options(int socket_fd, uint8_t ttl)
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
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) == -1)
        perror("WARNING: setsockopt(SO_BROADCAST)");
    if (setsockopt(socket_fd, IPPROTO_IP, IP_TTL, &(int){ttl}, sizeof(int)) == -1)
        perror("WARNING: setsockopt(IP_TTL)");
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
        if(ping->nb_packets_received == 0)
            return ;
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
    fprintf(stderr, "Usage: %s [OPTION...] HOST\n", name);
}

int parse_args(t_ping *ping, int argc, char **argv)
{
    int     opt;
    char    *error_ptr;
    long    option_value;

    static struct option long_options[] = {
    {"ttl", required_argument, NULL, 256},
    {NULL, 0, NULL, 0}
    };
    ping->verbose = 1;
    ping->iterations = UINT16_MAX;
    ping->interval_in_s = 1;
    ping->ttl = 64;
    ping->timeout_in_s = 0;
    while ((opt = getopt_long(argc, argv, "vc:qi:w:?", long_options, NULL)) != -1)
    {
        switch (opt) {
            case 'v': // verbose
                ping->verbose = 2;
                break;
            case 'q': // quiet
                ping->verbose = 0;
                break;
            case 'c': // number of packets to send
               option_value = strtol(optarg, &error_ptr, 10);
                if (*error_ptr != '\0' || option_value < 0 || option_value > UINT16_MAX)
                {
                    fprintf(stderr, "Invalid packet count: %s\n", optarg);
                    return -1;
                }
                if (option_value == 0)
                    break;
                ping->iterations = option_value;
                break;
            case 'i': // interval between packets
                option_value = strtol(optarg, &error_ptr, 10);
                if (*error_ptr != '\0' || option_value <= 0 || option_value > UINT64_MAX)
                {
                    fprintf(stderr, "Invalid interval: %s\n", optarg);
                    return -1;
                }
                ping->interval_in_s = option_value;
                break;
            case 256: // ttl
                option_value = strtol(optarg, &error_ptr, 10);
                if (*error_ptr != '\0' || option_value <= 0 || option_value > 255)
                {
                    fprintf(stderr, "Invalid ttl: %s\n", optarg);
                    return -1;
                }
                ping->ttl = option_value;
                break;
            case 'w': // timeout
                option_value = strtol(optarg, &error_ptr, 10);
                if (*error_ptr != '\0' || option_value <= 0 || option_value > UINT64_MAX)
                {
                    fprintf(stderr, "Invalid timeout: %s\n", optarg);
                    return -1;
                }
                ping->timeout_in_s = option_value;
                break;
            case '?':
                display_help(argv[0]);
                ping->iterations = 0;
                return (1);
            default:
                display_help(argv[0]);
                ping->iterations = 0;
                return (-1);
        }
    }
    if (optind >= argc)
    {
        fprintf(stderr, "ft_ping: missing host operand\n");
        fprintf(stderr, "Try %s '-?' for more information.\n", argv[0]);
        return (-1);
    }
    ping->dest_hostname = argv[optind];
    return (0);
}
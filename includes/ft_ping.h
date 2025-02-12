#ifndef FT_PING_H
# define FT_PING_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <arpa/inet.h> // check if needed
# include <netinet/ip_icmp.h>
# include <netinet/ip.h>
# include <sys/time.h>
# include <signal.h>
# include <time.h>
# include <stdbool.h>
# include <errno.h>
# include <getopt.h>

# define ICMP_FILTER	1
# define DEFAULT_ICMP_DATA_SIZE 56
# define ICMP_HEADER_SIZE sizeof(struct icmphdr)

typedef struct s_sequence
{
    int sequence;
    double request_time;
    uint16_t checksum;
    struct s_sequence *next;
} t_sequence;

typedef struct s_icmp_package
{
    struct icmphdr icmp_header;
    struct timeval timestamp;
    char data[DEFAULT_ICMP_DATA_SIZE-sizeof(struct timeval)];
} t_icmp_package;

typedef struct s_ping
{
	char *dest_hostname;
	char *dest_ip;
    int socket_fd;
    char received_buffer[(sizeof(struct iphdr) + sizeof(struct icmphdr) + DEFAULT_ICMP_DATA_SIZE) * 2]; // when error we receive the send packet
    t_icmp_package send_icmp_package;
    int nb_packets_send;
    int nb_packets_received;
    uint8_t ttl;
    int verbose;
    uint16_t iterations;
    int interval_in_s;
    int timeout_in_s;
    t_sequence *received_sequence;
} t_ping;

uint16_t	icmp_checksum(void *icmp, int len);
void create_icmp_package(t_ping *ping);
int extract_package(t_ping *ping, char *received_buffer, int len_received_ip_packet, double request_time);
void sleep_remaining_time(t_ping *ping, long start_time, long end_time);
void clean_ping(t_ping	*ping);
void print_packet_info(int sequence, int ttl, double request_time, int status);
void set_socket_options(int socket_fd, uint8_t ttl);
void print_statistics(t_ping *ping);
int parse_args(t_ping *ping, int argc, char **argv);
void print_ip_packet_resume(t_ping *ping, struct iphdr *ip_header);
void print_ip_and_icmp_details(struct iphdr *ip_header, struct icmphdr *icmp_header);

#endif

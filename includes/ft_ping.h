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
# include <stdbool.h>
# include <errno.h>

# define DEFAULT_ICMP_DATA_SIZE 56
# define ICMP_HEADER_SIZE sizeof(struct icmphdr)

typedef struct s_sequence
{
    int sequence;
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
    // char *received_buffer;
    char received_buffer[1024];
    t_icmp_package send_icmp_package;
    int nb_packets_send;
    int nb_packets_received;
    t_sequence *received_sequence;
} t_ping;

uint16_t	icmp_checksum(void *icmp, int len);
void create_icmp_package(t_ping *ping);
int extarct_package(t_ping *ping, char *received_buffer, int len_received_ip_packet, double request_time);
void sleep_remaining_time(long start_time, long end_time);
void clean_ping(t_ping	*ping);
void print_packet_info(t_ping *ping, int bytes_received, int sequence, int ttl, double request_time, int status);

#endif

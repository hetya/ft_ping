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

typedef struct s_ping
{
	char *dest_hostname;
	char *dest_ip;
    int socket_fd;
    // char *received_buffer;
    char received_buffer[1024];
    struct icmphdr send_icmp_header;
    struct iphdr *received_ip_header;
    struct icmphdr *received_icmp_header;
    int nb_packets_send;
    int nb_packets_received;
} t_ping;

uint16_t	icmp_checksum(struct icmphdr *icmp);
int create_icmp_package(t_ping *ping);
int extarct_package(t_ping *ping, char *received_buffer, int len_received_ip_packet);

#endif

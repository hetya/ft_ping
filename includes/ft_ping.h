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
# include <time.h>

typedef struct s_ping
{
	char *dest_hostname;
	char *dest_ip;
    int sockfd;
    struct icmphdr receive_icmp_header;
} t_ping;

#endif

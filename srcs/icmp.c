#include "ft_ping.h"

uint16_t	icmp_checksum(struct icmphdr *icmp)
{
	int			len;
	uint32_t	sum;
	uint16_t	one_complement;
	uint16_t	*ptr;

	ptr = (uint16_t *)icmp;
	sum = 0;
	for (len = sizeof(struct icmphdr); len > 1; len -= 2)
		sum += *ptr++;
	if (len == 1)
		sum += *(unsigned char *)ptr;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	one_complement = ~sum;
	return (one_complement);
}

int create_icmp_package(t_ping *ping)
{
    ping->send_icmp_header.type = ICMP_ECHO;
	ping->send_icmp_header.code = 0;
	ping->send_icmp_header.un.echo.id = getpid();
	ping->send_icmp_header.un.echo.sequence = 0;
}

int extarct_package(t_ping *ping, char *received_buffer, int len_received_ip_packet)
{
	uint16_t received_checksum;

    struct iphdr *ip_header = (struct iphdr *)received_buffer;
	int ip_header_len = ip_header->ihl * 4;
    if (ip_header->protocol != IPPROTO_ICMP)
        return (1);
    if (ip_header->saddr != inet_addr(ping->dest_ip))
        return (1);

	// ICMP header starts after IP header
    struct icmphdr *icmp_header = (struct icmphdr *)(received_buffer + ip_header_len);
    if (icmp_header->type != ICMP_ECHOREPLY)
		return (1);
	if (icmp_header->un.echo.id != ping->send_icmp_header.un.echo.id)
		return (1);
	if (icmp_header->un.echo.sequence != ping->send_icmp_header.un.echo.sequence)
		return (1);
	received_checksum = icmp_header->checksum;
	icmp_header->checksum = 0;
	if (icmp_checksum(icmp_header) != received_checksum)
		return (1);
	ping->received_ip_header = ip_header;
	ping->received_icmp_header = icmp_header;
	ping->nb_packets_received++;
	return (0);
}

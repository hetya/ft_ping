#include "ft_ping.h"

uint16_t	icmp_checksum(void *struct_ptr, int len)
{
	uint32_t	sum;
	uint16_t	one_complement;
	uint16_t	*ptr;

	ptr = (uint16_t *)struct_ptr;
	sum = 0;
	for (; len > 1; len -= 2)
		sum += *ptr++;
	if (len == 1)
		sum += *(unsigned char *)ptr;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	one_complement = ~sum;
	return (one_complement);
}

void create_icmp_package(t_ping *ping)
{
    ping->send_icmp_package.icmp_header.type = ICMP_ECHO;
	ping->send_icmp_package.icmp_header.code = 0;
	ping->send_icmp_package.icmp_header.un.echo.id = getpid();
	ping->send_icmp_package.icmp_header.un.echo.sequence = 0;
}

int check_and_add_sequence(t_ping *ping, int sequence)
{
	t_sequence *tmp;
	t_sequence *new_sequence;

	tmp = ping->received_sequence;
	while (tmp && tmp->next)
	{
		if (tmp->sequence == sequence)
			return (1);
		tmp = tmp->next;
	}
	new_sequence = malloc(sizeof(t_sequence));
	if (!new_sequence)
	{
		perror("malloc");
		clean_ping(ping);
		exit(1);
	}
	new_sequence->sequence = sequence;
	new_sequence->next = NULL;
	if (!ping->received_sequence)
		ping->received_sequence = new_sequence;
	else
		tmp->next = new_sequence;
	return (0);
}

int extarct_package(t_ping *ping, char *received_buffer, int len_received_ip_packet, double request_time)
{
	uint16_t received_checksum;
	int status = 0;

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
	if (icmp_header->un.echo.id != ping->send_icmp_package.icmp_header.un.echo.id)
		return (1);
	if (icmp_header->un.echo.sequence != ping->send_icmp_package.icmp_header.un.echo.sequence)
		return (1);
	received_checksum = icmp_header->checksum;
	icmp_header->checksum = 0;
	if (icmp_checksum(icmp_header, ntohs(ip_header->tot_len) - (ip_header->ihl * 4)) != received_checksum)
		status = 1;
	if (check_and_add_sequence(ping, icmp_header->un.echo.sequence) == 1)
		status = 2;
	print_packet_info(ping, ntohs(ip_header->tot_len) - (ip_header->ihl * 4), icmp_header->un.echo.sequence, ip_header->ttl, request_time, status);
	ping->nb_packets_received++;
	return (status);
}

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

int check_and_add_sequence(t_ping *ping, int sequence, double request_time)
{
	t_sequence *list;
	t_sequence *new_sequence;

	list = ping->received_sequence;
	while (list && list->next)
	{
		if (list->sequence == sequence)
			return (1);
		list = list->next;
	}
	new_sequence = malloc(sizeof(t_sequence));
	if (!new_sequence)
	{
		perror("malloc");
		clean_ping(ping);
		exit(1);
	}
	new_sequence->sequence = sequence;
	new_sequence->request_time = request_time;
	new_sequence->next = NULL;
	if (!ping->received_sequence)
		ping->received_sequence = new_sequence;
	else
		list->next = new_sequence;
	return (0);
}

int check_icmp_type(int type)
{
	switch (type)
	{
		case ICMP_ECHOREPLY:
			return (0);
		case ICMP_DEST_UNREACH:
			printf("Destination Unreachable\n");
			return (1);
		case ICMP_SOURCE_QUENCH:
			printf("Source Quench\n");
			return (1);
		case ICMP_REDIRECT:
			printf("Redirect\n");
			return (1);
		case ICMP_TIME_EXCEEDED:
			printf("Time To Live Exceeded\n");
			return (1);
		case ICMP_PARAMETERPROB:
			printf("Parameter Problem\n");
			return (1);
		default:
			return (1);
	}
}

int extarct_package(t_ping *ping, char *received_buffer, int len_received_ip_packet, double request_time)
{
	uint16_t tmp_checksum;
	int status = 0;

	if (len_received_ip_packet < (int)(sizeof(struct iphdr) + sizeof(struct icmphdr)))
		return (1);
    struct iphdr *ip_header = (struct iphdr *)received_buffer;
	int ip_header_len = ip_header->ihl * 4;
	tmp_checksum = ip_header->check;
	ip_header->check = 0;
	if (icmp_checksum(received_buffer, len_received_ip_packet) != tmp_checksum)
		return (1);
    if (ip_header->protocol != IPPROTO_ICMP)
        return (1);
    if (ip_header->saddr != inet_addr(ping->dest_ip))
        return (1);

	// ICMP header starts after IP header
	struct icmphdr *icmp_header = (struct icmphdr *)(received_buffer + ip_header_len);
	printf("%d bytes from %s", ntohs(ip_header->tot_len) - (ip_header->ihl * 4), ping->dest_hostname);
	if (strcmp(ping->dest_hostname, ping->dest_ip))
		printf(" (%s)", ping->dest_ip);
	printf(": ");

	if (check_icmp_type(icmp_header->type) == 1)
		return (1);
	if (icmp_header->un.echo.id != ping->send_icmp_package.icmp_header.un.echo.id)
		return (1);
	if (check_and_add_sequence(ping, icmp_header->un.echo.sequence, request_time) == 1)
		status = 2;
	// todo : patch this case
	// if (icmp_header->un.echo.sequence != ping->send_icmp_package.icmp_header.un.echo.sequence)
	// 	return (1);
	tmp_checksum = icmp_header->checksum;
	icmp_header->checksum = 0;
	if (icmp_checksum(icmp_header, ntohs(ip_header->tot_len) - (ip_header->ihl * 4)) != tmp_checksum)
		status = 1;
	print_packet_info(icmp_header->un.echo.sequence, ip_header->ttl, request_time, status);
	ping->nb_packets_received++;
	return (status);
}

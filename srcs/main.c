/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unknown <unknown>                          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/28 16:47:03 by unknown           #+#    #+#             */
/*   Updated: 2025/02/03 17:18:51 by unknown          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

int	create_socket_and_connect(char *hostname)
{
	struct addrinfo	hints;
	struct addrinfo	*res;
	struct addrinfo	*address_list;
	int				sockfd;
	int				status;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;
	status = getaddrinfo(hostname, NULL, &hints, &res);
	if (status != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return (1);
	}
	address_list = res;
	while (address_list != NULL)
	{
		// Uttiliser so_broadcast
		sockfd = socket(address_list->ai_family, address_list->ai_socktype,
				address_list->ai_protocol);
		if (sockfd == -1)
		{
			perror("socket");
			address_list = address_list->ai_next;
			continue ;
		}
		if (connect(sockfd, address_list->ai_addr, address_list->ai_addrlen)
			== -1)
		{
			perror("connect");
			close(sockfd);
			address_list = address_list->ai_next;
			continue ;
		}
		break ;
		address_list = address_list->ai_next;
	}
	if (address_list == NULL)
	{
		fprintf(stderr, "Can't resolve hostname %s\n", hostname);
		freeaddrinfo(res);
		return (1);
	}
	freeaddrinfo(res);
	printf("PING %s \n", hostname);
	return (sockfd);
}

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


int	main(int argc, char **argv)
{
	int				socket_fd;
	struct icmphdr	icmp_header;
	int				len_received_ip_packet;

	if (getuid() != 0)
	{
		fprintf(stderr, "You must use root privilege to run this program.\n");
		return (1);
	}
	if (argc < 2)
	{
		printf("Usage: %s <hostname>\n", argv[0]);
		return (1);
	}
	socket_fd = create_socket_and_connect(argv[1]);
	icmp_header.type = ICMP_ECHO;
	icmp_header.code = 0;
	icmp_header.un.echo.id = getpid();
	icmp_header.un.echo.sequence = 0;
	icmp_header.checksum = 0;
	icmp_header.checksum = icmp_checksum(&icmp_header);
	// data
	while(1)
	{
		printf("Packet :\n icmp_type=%d\n icmp_code=%d\n icmp_checksum=%d\n icmp_id=%d\n icmp_seq=%d\n",
			icmp_header.type, icmp_header.code, icmp_header.checksum, icmp_header.un.echo.id, icmp_header.un.echo.sequence);
		if (send(socket_fd, &icmp_header, sizeof(icmp_header), 0) == -1)
		{
			perror("send");
			return (1);
		}
		printf("Sent ICMP packet\n");
		char receive_buffer[1024];
		len_received_ip_packet = recv(socket_fd, receive_buffer, sizeof(receive_buffer), 0);
		if (len_received_ip_packet == -1)
		{
			perror("recv");
			return (1);
		}
		printf("Received IP packet: len %d\n");
		printf("=====\n", len_received_ip_packet);
		write(1, receive_buffer, len_received_ip_packet);
		printf("\n=====\n");
		// printf("Received ICMP packet icmp_seq=%d\n", icmp_header.un.echo.sequence);
		// icmp_header.un.echo.sequence++;
		printf("icmp_seq=%d\n", icmp_header.un.echo.sequence);
		sleep(1);
	}
	// duplicate package
	printf("ICMP size: %ld\n", sizeof(icmp_header));
	send(socket_fd, &icmp_header, sizeof(icmp_header), 0);

	close(socket_fd);
}

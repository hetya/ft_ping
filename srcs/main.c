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

t_ping *g_ping = 0;

int	create_socket_and_connect(t_ping *ping)
{
	struct addrinfo	hints;
	struct addrinfo	*res;
	struct addrinfo	*address_list;
	int				status;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;
	status = getaddrinfo(ping->dest_hostname, NULL, &hints, &res);
	if (status != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return (1);
	}
	address_list = res;
	while (address_list != NULL)
	{
		// Uttiliser so_broadcast
		ping->socket_fd = socket(address_list->ai_family, address_list->ai_socktype,
				address_list->ai_protocol);
		if (ping->socket_fd == -1)
		{
			address_list = address_list->ai_next;
			continue ;
		}
		if (setsockopt(ping->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &(struct timeval){1, 0}, sizeof(struct timeval)) == -1)
		{
			address_list = address_list->ai_next;
			continue ;
		}
		if (connect(ping->socket_fd, address_list->ai_addr, address_list->ai_addrlen)
			== -1)
		{
			perror("connect");
			address_list = address_list->ai_next;
			continue ;
		}
		ping->dest_ip = inet_ntoa(((struct sockaddr_in *)address_list->ai_addr)->sin_addr);
		break ;
	}
	if (address_list == NULL)
	{
		fprintf(stderr, "Can't resolve hostname %s\n", ping->dest_hostname);
		freeaddrinfo(res);
		return (-1);
	}
	freeaddrinfo(res);
	return (ping->socket_fd);
}

void	handle_exit(int sig)
{
	if (sig == SIGINT)
	{
		if(g_ping)
		{
			if (g_ping->nb_packets_send)
			{
				printf("\n--- %s ping statistics ---\n", g_ping->dest_hostname);
				printf("%d packets transmitted, %d received, %d%% packet loss\n",
					g_ping->nb_packets_send, g_ping->nb_packets_received, (g_ping->nb_packets_send - g_ping->nb_packets_received) * 100 / g_ping->nb_packets_send);
			}
			clean_ping(g_ping);
			exit(0);
		}
	}
}

int	main(int argc, char **argv)
{
	int				len_received_ip_packet;
	double			send_time;
	double			receive_time;
	struct timeval 	tv;
	t_ping 			*ping;


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
	signal(SIGINT, handle_exit);
	ping = malloc(sizeof(t_ping));
	if (!ping)
	{
		perror("malloc");
		return (1);
	}
	g_ping = ping;
	memset(ping, 0, sizeof(t_ping));
	ping->socket_fd = -1;
	ping->dest_hostname = argv[1];
	if (create_socket_and_connect(ping) == -1)
	{
		clean_ping(ping);
		return (1);
	}
	create_icmp_package(ping);
	printf("PING %s (%s) %d(%ld) bytes of data.\n", ping->dest_hostname, ping->dest_ip, DEFAULT_ICMP_DATA_SIZE, DEFAULT_ICMP_DATA_SIZE + sizeof(struct iphdr) + sizeof(struct icmphdr));
	// check ip checksum
	// check for mal formed or duplicate package
	// bonus ttl + -s
	for(int i = 0; i < UINT16_MAX; i++)
	{
		ping->send_icmp_package.icmp_header.un.echo.sequence++;
		// recreate the checksum since the sequence number has changed
		gettimeofday(&tv, NULL);
		ping->send_icmp_package.timestamp = tv;
		ping->send_icmp_package.icmp_header.checksum = 0;
		ping->send_icmp_package.icmp_header.checksum = icmp_checksum(&ping->send_icmp_package, sizeof(ping->send_icmp_package));
		send_time = tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
		if (send(ping->socket_fd, &ping->send_icmp_package, sizeof(ping->send_icmp_package), 0) == -1)
		{
			perror("send");
			clean_ping(ping);
			return (1);
		}
		ping->nb_packets_send++;
		len_received_ip_packet = recv(ping->socket_fd, ping->received_buffer, sizeof(ping->received_buffer), 0);
		if (len_received_ip_packet == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				printf("Request timeout for icmp_seq %d\n", ping->send_icmp_package.icmp_header.un.echo.sequence);
				continue ;
			}
			perror("recv");
			clean_ping(ping);
			return (1);
		}
		gettimeofday(&tv, NULL);
		receive_time = tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
		if (extarct_package(ping, ping->received_buffer, len_received_ip_packet))
		{
			continue ;
		}
		printf("%d bytes from %s", ntohs(ping->received_ip_header->tot_len) - (ping->received_ip_header->ihl * 4), argv[1]);
		if (strcmp(ping->dest_hostname, ping->dest_ip))
			printf(" (%s)", inet_ntoa(*(struct in_addr *)&ping->received_ip_header->saddr));
		printf(": icmp_seq=%d ttl=%d time=%.1f ms\n",
			ping->received_icmp_package.icmp_header.un.echo.sequence, ping->received_ip_header->ttl, receive_time - send_time);
		sleep_remaining_time(send_time, receive_time);
	}
}

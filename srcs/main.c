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

bool g_exit = 0;

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
			perror("socket");
			address_list = address_list->ai_next;
			continue ;
		}
		if (connect(ping->socket_fd, address_list->ai_addr, address_list->ai_addrlen)
			== -1)
		{
			perror("connect");
			close(ping->socket_fd);
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
		return (1);
	}
	freeaddrinfo(res);
	return (ping->socket_fd);
}

void	handle_exit(int sig)
{
	if (sig == SIGINT)
	{
		g_exit = 1;
	}
}

int	main(int argc, char **argv)
{
	int				len_received_ip_packet;
	double			send_time;
	double			receive_time;
	int				nb_packets_sent;
	int				nb_packets_received;
	t_ping 			ping;


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
	memset(&ping, 0, sizeof(ping));
	// signal(SIGINT, handle_exit);
	ping.socket_fd = -1;
	ping.dest_hostname = argv[1];
	create_socket_and_connect(&ping);
	create_icmp_package(&ping);
	printf("PING %s (%s).\n", ping.dest_hostname, ping.dest_ip);
	// data + size buf
	// check internet checksum + duplicate package
	// queue for recv?
	// close fd when ctrl+c
	nb_packets_sent = 0;
	nb_packets_received = 0;
	while(!g_exit)
	{
		// recreate the checksum since the sequence number has changed
		ping.send_icmp_header.checksum = 0;
		ping.send_icmp_header.checksum = icmp_checksum(&ping.send_icmp_header);
		send_time = get_time_in_ms();
		if (send(ping.socket_fd, &ping.send_icmp_header, sizeof(ping.send_icmp_header), 0) == -1)
		{
			perror("send");
			return (1);
		}
		nb_packets_sent++;
		len_received_ip_packet = recv(ping.socket_fd, ping.receive_buffer, sizeof(ping.receive_buffer), 0);
		if (len_received_ip_packet == -1)
		{
			perror("recv");
			return (1);
		}
		nb_packets_received++;
		receive_time = get_time_in_ms();
		extarct_package(&ping, ping.receive_buffer, len_received_ip_packet);
		printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%.1f ms\n",
			len_received_ip_packet, argv[1], inet_ntoa(*(struct in_addr *)&ping.received_ip_header->saddr), ping.received_icmp_header->un.echo.sequence, ping.received_ip_header->ttl, receive_time - send_time);
		ping.send_icmp_header.un.echo.sequence++;
		sleep(1);
	}
	printf("\n--- %s ping statistics ---\n", argv[1]);
	printf("%d packets transmitted, %d received, %d%% packet loss\n",
		nb_packets_sent, nb_packets_received, (nb_packets_sent - nb_packets_received) * 100 / nb_packets_sent);
	close(ping.socket_fd);
}

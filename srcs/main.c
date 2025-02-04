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
	struct timeval 	tv;
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
	ping.socket_fd = -1;
	ping.dest_hostname = argv[1];
	create_socket_and_connect(&ping);
	create_icmp_package(&ping);
	printf("PING %s (%s).\n", ping.dest_hostname, ping.dest_ip);
	printf("sizeof(ping.send_icmp_header) = %ld\n", sizeof(ping.send_icmp_header) + sizeof(struct iphdr));
	printf("sizeof = %ld\n", sizeof(ping.send_icmp_header)+ sizeof(tv));
	// data + size buf
	// check ip checksum + duplicate package
	// queue for recv?
	// check for mal formed or duplicate package
	while(!g_exit)
	{
		// recreate the checksum since the sequence number has changed
		gettimeofday(&tv, NULL);
		ping.send_icmp_header.checksum = 0;
		ping.send_icmp_header.checksum = icmp_checksum(&ping.send_icmp_header);
		send_time = tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
		if (send(ping.socket_fd, &ping.send_icmp_header, sizeof(ping.send_icmp_header), 0) == -1)
		{
			perror("send");
			close(ping.socket_fd);
			return (1);
		}
		ping.nb_packets_send++;
		len_received_ip_packet = recv(ping.socket_fd, ping.received_buffer, sizeof(ping.received_buffer), 0);
		if (len_received_ip_packet == -1)
		{
			perror("recv");
			close(ping.socket_fd);
			return (1);
		}
		gettimeofday(&tv, NULL);
		receive_time = tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
		if (extarct_package(&ping, ping.received_buffer, len_received_ip_packet))
		{
			continue ;
		}
		printf("%d bytes from %s", len_received_ip_packet, argv[1]); // todo change the size
		if (strcmp(ping.dest_hostname, ping.dest_ip))
			printf(" (%s)", inet_ntoa(*(struct in_addr *)&ping.received_ip_header->saddr));

		printf(": icmp_seq=%d ttl=%d time=%.1f ms\n",
			ping.received_icmp_header->un.echo.sequence, ping.received_ip_header->ttl, receive_time - send_time);
		ping.send_icmp_header.un.echo.sequence++;
		sleep(1);
	}
	printf("\n--- %s ping statistics ---\n", argv[1]);
	printf("%d packets transmitted, %d received, %d%% packet loss\n",
		ping.nb_packets_send, ping.nb_packets_received, (ping.nb_packets_send - ping.nb_packets_received) * 100 / ping.nb_packets_send);
	close(ping.socket_fd);
}

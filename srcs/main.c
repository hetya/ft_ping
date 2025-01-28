/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unknown <unknown>                          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/28 16:47:03 by unknown           #+#    #+#             */
/*   Updated: 2025/01/28 22:25:52 by unknown          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

int	create_socket(char *hostname)
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
	if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return (1);
	}

	address_list = res;
	while (address_list != NULL)
	{
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
		printf("Connected successfully to %s\n", hostname);
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
	printf("Connection established. Ready to send/receive data.\n");

	return (sockfd);
}

int	main(int argc, char **argv)
{
	int	socket_fd;

	if (argc < 2)
	{
		printf("Usage: %s <hostname>\n", argv[0]);
		return (1);
	}
	socket_fd = create_socket(argv[1]);

	close(socket_fd);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unknown <unknown>                          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/28 16:47:03 by unknown           #+#    #+#             */
/*   Updated: 2025/02/10 22:01:55 by unknown          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

t_ping *g_ping = 0;

int parse_args(t_ping *ping, int argc, char **argv)
{
    int		opt;
    char	*error_ptr;
    long	option_value;

    static struct option long_options[] = {
    {"count", required_argument, NULL, 'c'},
    {"interval", required_argument, NULL, 'i'},
    {"ttl", required_argument, NULL, 256},
    {"verbose", no_argument, NULL, 'v'},
    {"quiet", no_argument, NULL, 'q'},
    {"timeout", required_argument, NULL, 'w'},
    {"help", no_argument, NULL, '?'},
    {NULL, 0, NULL, 0}
    };
    ping->verbose = 0;
    ping->iterations = UINT16_MAX;
    ping->interval_in_s = 1;
    ping->ttl = 64;
    ping->timeout_in_s = 0;
    while ((opt = getopt_long(argc, argv, "vc:qi:w:?", long_options, NULL)) != -1)
    {
        switch (opt) {
            case 'v': // verbose
                ping->verbose |= FLAG_VERBOSE;
                break;
            case 'q': // quiet
                ping->verbose |= FLAG_QUIET;
                break;
            case 'c': // number of packets to send
               option_value = strtol(optarg, &error_ptr, 10);
                if (*error_ptr != '\0' || option_value < 0 || option_value > UINT16_MAX)
                {
                    fprintf(stderr, "Invalid packet count: %s\n", optarg);
                    return (-1);
                }
                if (option_value == 0)
                    break;
                ping->iterations = option_value;
                break;
            case 'i': // interval between packets
                option_value = strtol(optarg, &error_ptr, 10);
                if (*error_ptr != '\0' || option_value <= 0 || option_value > UINT64_MAX)
                {
                    fprintf(stderr, "Invalid interval: %s\n", optarg);
                    return (-1);
                }
                ping->interval_in_s = option_value;
                break;
            case 256: // ttl
                option_value = strtol(optarg, &error_ptr, 10);
                if (*error_ptr != '\0' || option_value <= 0 || option_value > 255)
                {
                    fprintf(stderr, "Invalid ttl: %s\n", optarg);
                    return (-1);
                }
                ping->ttl = option_value;
                break;
            case 'w': // timeout
                option_value = strtol(optarg, &error_ptr, 10);
                if (*error_ptr != '\0' || option_value <= 0 || option_value > UINT64_MAX)
                {
                    fprintf(stderr, "Invalid timeout: %s\n", optarg);
                    return (-1);
                }
                ping->timeout_in_s = option_value;
                break;
            case '?':
                display_help(argv[0]);
                ping->iterations = 0;
                return (1);
        }
    }
    if (optind >= argc)
    {
        fprintf(stderr, "ft_ping: missing host operand\n");
        fprintf(stderr, "Try %s '-?' for more information.\n", argv[0]);
        return (-1);
    }
    ping->dest_hostname = argv[optind];
    return (0);
}

int	resolve_and_create_socket(t_ping *ping)
{
	struct addrinfo	hints;
	struct addrinfo	*res;
	struct addrinfo	*address_list;
	int				status;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;
	status = getaddrinfo(ping->dest_hostname, NULL, &hints, &res);
	if (status != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return (-1);
	}
	address_list = res;
	while (address_list != NULL)
	{
		ping->socket_fd = socket(address_list->ai_family, address_list->ai_socktype,
				address_list->ai_protocol);
		if (ping->socket_fd == -1)
		{
			address_list = address_list->ai_next;
			continue ;
		}
		set_socket_options(ping->socket_fd, ping->ttl);
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
			print_statistics(g_ping);
			clean_ping(g_ping);
			exit(0);
		}
	}
}

int	main(int argc, char **argv)
{
	int					len_received_ip_packet;
	double				send_time;
	double				receive_time;
	struct timeval 		start_tv;
	struct timeval 		tmp_tv;
	t_ping 				*ping;
	struct sockaddr_in	dest_addr;
	struct sockaddr_in	reply_addr;

	if (getuid() != 0)
	{
		fprintf(stderr, "You must use root privilege to run this program.\n");
		return (1);
	}
	signal(SIGINT, handle_exit);
	ping = malloc(sizeof(t_ping));
	if (!ping)
	{
		perror("malloc");
		return (1);
	}
	memset(ping, 0, sizeof(t_ping));
	g_ping = ping;
	ping->socket_fd = -1;
	if (parse_args(ping, argc, argv))
	{
		clean_ping(ping);
		return (1);
	}
	if (resolve_and_create_socket(ping) == -1)
	{
		clean_ping(ping);
		return (1);
	}
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = 0;
	dest_addr.sin_addr.s_addr = inet_addr(ping->dest_ip);
	socklen_t addr_len = sizeof(reply_addr);
	create_icmp_package(ping);
	printf("PING %s (%s): %d data bytes", ping->dest_hostname, ping->dest_ip, DEFAULT_ICMP_DATA_SIZE);
	if (ping->verbose & FLAG_VERBOSE)
		printf(", id 0x%x = %d", ping->send_icmp_package.icmp_header.un.echo.id, ping->send_icmp_package.icmp_header.un.echo.id);
	printf("\n");
	gettimeofday(&start_tv, NULL);
	for(int i = 0; i < ping->iterations; i++)
	{
		// recreate the checksum since the sequence number has changed
		gettimeofday(&tmp_tv, NULL);
		ping->send_icmp_package.timestamp = tmp_tv;
		ping->send_icmp_package.icmp_header.checksum = 0;
		ping->send_icmp_package.icmp_header.checksum = icmp_checksum(&ping->send_icmp_package, sizeof(ping->send_icmp_package));
		send_time = tmp_tv.tv_sec * 1000.0 + tmp_tv.tv_usec / 1000.0;
		if (sendto(ping->socket_fd, &ping->send_icmp_package, sizeof(ping->send_icmp_package), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1)
		{
			perror("send");
			clean_ping(ping);
			return (1);
		}
		ping->nb_packets_send++;
		len_received_ip_packet = recvfrom(ping->socket_fd, ping->received_buffer, sizeof(ping->received_buffer), 0,(struct sockaddr *)&reply_addr, &addr_len);
		if (len_received_ip_packet == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue ;
			perror("recv");
			clean_ping(ping);
			return (1);
		}
		gettimeofday(&tmp_tv, NULL);
		receive_time = tmp_tv.tv_sec * 1000.0 + tmp_tv.tv_usec / 1000.0;
		extract_package(ping, ping->received_buffer, len_received_ip_packet, receive_time - send_time);
		sleep_remaining_time(ping, send_time, receive_time);
		gettimeofday(&tmp_tv, NULL);
		if (ping->timeout_in_s != 0 && tmp_tv.tv_sec - start_tv.tv_sec > ping->timeout_in_s)
			break ;
		ping->send_icmp_package.icmp_header.un.echo.sequence++;
	}
	print_statistics(g_ping);
	clean_ping(g_ping);
	exit(0);
}

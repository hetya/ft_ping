#include "ft_ping.h"

void set_socket_options(int socket_fd, uint8_t ttl)
{
	uint32_t	data;
	
	// Filter out unwanted ICMP packets
	data = ~((1<<ICMP_SOURCE_QUENCH)|
	(1<<ICMP_DEST_UNREACH)|
	(1<<ICMP_TIME_EXCEEDED)|
	(1<<ICMP_PARAMETERPROB)|
	(1<<ICMP_REDIRECT)|
	(1<<ICMP_ECHOREPLY));
	if (setsockopt(socket_fd, SOL_RAW, ICMP_FILTER, (char*)&data, sizeof(data)) == -1)
		perror("WARNING: setsockopt(ICMP_FILTER)");
	if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &(struct timeval){1, 0}, sizeof(struct timeval)) == -1)
		perror("WARNING: setsockopt(SO_RCVTIMEO)");
	if (setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) == -1)
		perror("WARNING: setsockopt(SO_BROADCAST)");
	if (setsockopt(socket_fd, IPPROTO_IP, IP_TTL, &(int){ttl}, sizeof(int)) == -1)
		perror("WARNING: setsockopt(IP_TTL)");
}

void sleep_remaining_time(t_ping *ping, long start_time, long end_time)
{
	struct timespec	ts;

	// calculate remaining time (1s - time to recv packet)
	long remaining_time_ns = 1000000000 - (start_time * 1000000 - end_time * 1000000);

	if (remaining_time_ns < 0)
		return ;
	ts.tv_sec = remaining_time_ns / 1000000000;
	ts.tv_nsec = remaining_time_ns % 1000000000;
	if (nanosleep(&ts, NULL) == -1) {
		perror("nanosleep failed");
	}
	if (ping->interval_in_s > 1)
		sleep(ping->interval_in_s - 1);
}

double ft_sqrt(double num)
{
	double	guess;
	double	epsilon;

	guess = num / 2.0;
	epsilon = 1e-3; // Precision level
	if (num < 0)
		return (-1);
	if (num == 0)
        return (0);
    while (fabs(guess * guess - num) > epsilon)
        guess = (guess + num / guess) / 2.0;
	return (guess);
}

void clean_ping(t_ping	*ping)
{
	t_sequence *next;

	if (ping)
	{
		if (ping->socket_fd != -1)
			close(ping->socket_fd);
		if (ping->received_sequence)
		{
			t_sequence *tmp = ping->received_sequence;
			while (tmp)
			{
				next = tmp->next;
				free(tmp);
				tmp = next;
			}
		}
		free(ping);
		ping = NULL;
	}
}

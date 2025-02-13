#include "ft_ping.h"

void print_ip_packet_resume(t_ping *ping, struct iphdr *ip_header)
{
    printf("%d bytes from %s", ntohs(ip_header->tot_len) - (ip_header->ihl * 4), ping->dest_hostname);
	if (strcmp(ping->dest_hostname, ping->dest_ip))
		printf(" (%s)", inet_ntoa((struct in_addr){ip_header->saddr}));
	printf(": ");
}

void print_packet_info(int sequence, int ttl, double request_time, int status)
{

    printf("icmp_seq=%d ttl=%d time=%.1f ms",
        sequence, ttl, request_time);
    if (status == 5)
        printf(" (malformed packet)");
    else if (status == 3)
        printf(" (duplicate packet)");
    printf("\n");
}

void print_ip_and_icmp_details(struct iphdr *ip_header, struct icmphdr *icmp_header)
{
    printf("IP Hdr Dump:\n");
    for (int i = 0; i < 20; i++)
    {
        printf("%02x", ((unsigned char *)ip_header)[i]);
        if (i % 2)
            printf(" ");
    }
    printf("\n");
    printf("Vr HL TOS Len  ID  Flg  off  TTL Pro cks  Src          Dst\n");
    printf("%d  %d  %02x %04x %04x %04x %04x %02x  %02x  %04x %s %s\n",
        ip_header->version, ip_header->ihl, ip_header->tos, ntohs(ip_header->tot_len), ntohs(ip_header->id), ntohs(ip_header->frag_off) >> 13, ntohs(ip_header->frag_off) & 0x1FFF, ip_header->ttl, ip_header->protocol, ntohs(ip_header->check), inet_ntoa((struct in_addr){ip_header->saddr}), inet_ntoa((struct in_addr){ip_header->daddr}));
    printf("ICMP: type %d, code %d, size %d, id 0x%04x, seq 0x%04x\n",
        icmp_header->type, icmp_header->code, ntohs(ip_header->tot_len) - (ip_header->ihl * 4), icmp_header->un.echo.id, icmp_header->un.echo.sequence);
}

void print_statistics(t_ping *ping)
{
    double min_rtt;
    double max_rtt;
    double sum_rtt;
    double mean_rtt;
    double stddev_rtt;

    min_rtt = -1;
    max_rtt = 0;
    if (ping->nb_packets_send)
    {
        printf("\n--- %s ping statistics ---\n", ping->dest_hostname);
        printf("%d packets transmitted, %d received, %d%% packet loss\n",
            ping->nb_packets_send, ping->nb_packets_received, (ping->nb_packets_send - ping->nb_packets_received) * 100 / ping->nb_packets_send);
        if(ping->nb_packets_received == 0)
            return ;
        for (t_sequence *tmp = ping->received_sequence; tmp; tmp = tmp->next)
        {
            if (min_rtt < 0 || tmp->request_time < min_rtt)
                min_rtt = tmp->request_time;
            if (tmp->request_time > max_rtt)
                max_rtt = tmp->request_time;
            sum_rtt += tmp->request_time;
        }
        if (min_rtt < 0)
            min_rtt = 0;
        mean_rtt = sum_rtt / (double)ping->nb_packets_received;
        stddev_rtt = 0;
        for (t_sequence *tmp = ping->received_sequence; tmp; tmp = tmp->next)
        {
            stddev_rtt += (tmp->request_time - mean_rtt) * (tmp->request_time - mean_rtt);
        }
        stddev_rtt = ft_sqrt(stddev_rtt / (double)ping->nb_packets_received);
        if (stddev_rtt < 0)
            fprintf(stderr, "Error while calculating round-trip");
        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
        min_rtt, mean_rtt, max_rtt, stddev_rtt);
    }
}

void display_help(char *name)
{
    printf("Usage: %s [OPTION...] HOST\n", name);
	printf("Options valid for all request types:\n\n");

	printf("-c, --count=NUMBER         stop after sending NUMBER packets\n");
	printf("-i, --interval=NUMBER      wait NUMBER seconds between sending each packet\n");
	printf("    --ttl=N                specify N as time-to-live\n");
	printf("-v, --verbose              verbose output\n");
	printf("-q, --quiet                quiet output\n");
	printf("-w, --timeout=NUMBER       stop after N seconds\n");
	printf("-?, --help                 give this help list\n");

}

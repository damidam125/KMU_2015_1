#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/types.h>
#include <errno.h>
//#include <netinet/tcp.h>
#include <sys/file.h>

#ifdef LINUX
#include <linux/unistd.h>
#endif // LINUX

#ifdef __NR_gettid
#else // __NR_gettid
  #define __NR_gettid 224
#endif // __NR_gettid

#ifndef MIN
#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#endif  /* MIN */

#include "mSnort.h"

mStat_t		stat;

void help ()
{
	printf ("mSnort Ver 0.1.0\n"
			"Usage: ./mSnort -i [Interface] -c [COUNT] -f [FILE]\n"
			"  -h                  Help\n"
			"  -i [INTERFACE]      Interface    ex) eth0\n"
			"  -f [FILE]           File output  ex) a.txt\n");
	exit (0);
}

void print_asc (unsigned char *buf,int len)
{
  int i;
  for (i = 0; i < len; i++)
    fprintf(stderr, "%c",isprint(buf[i])?buf[i]:'.');
}

void Dump_Data (unsigned char *buf, int len)
{
  int i = 0;
  if (len <= 0) return;
  fprintf(stderr, "[%03X] ", i);
  for (i = 0; i < len;) {
    fprintf(stderr, "%02X ", (int)buf[i]);
    i++;
    if (i%8 == 0) fprintf(stderr, " ");
    if (i%16 == 0) {
      print_asc(&buf[i-16], 8); fprintf(stderr, " ");
      print_asc(&buf[i-8], 8); fprintf(stderr, "\n");
      if (i < len) fprintf(stderr, "[%03X] ",i);
    }
  }
  if (i%16) {
    int n;

    n = 16 - (i%16);
    fprintf(stderr, " ");
    if (n > 8) fprintf(stderr, " ");
    while (n--) fprintf(stderr, "   ");

    n = MIN(8,i%16);
    print_asc(&buf[i-(i%16)],n); fprintf(stderr, " ");
    n = (i%16) - n;
    if (n > 0) print_asc(&buf[i-n],n);
    fprintf(stderr, "\n");
  }
    fprintf(stderr, "\n");
}

int enable_core ()
{
	int n;
  struct rlimit rl;
	if (getrlimit (RLIMIT_NOFILE, &rl) == -1){
		return -1;
	}
	n = rl.rlim_cur = rl.rlim_max;
	setrlimit (RLIMIT_NOFILE, &rl);

  if (getrlimit (RLIMIT_CORE, &rl) == -1){
		return -1;
	}
	rl.rlim_cur = rl.rlim_max;
  setrlimit (RLIMIT_CORE, &rl);

	return 0;
}

int decode (mPacket_t *p, char *pkt, unsigned short plen)
{
	struct ip 			*iph = (struct ip *)(pkt + sizeof(struct ether_header));
	struct tcphdr 	*th = NULL;
	unsigned int 	ihlen = iph->ip_hl * 4;
	unsigned int 	thlen = 0;
	unsigned int	offset;
	unsigned int  length;

	if (iph->ip_v != 4) return -1;
	if (iph->ip_off & htons(IP_MF|IP_OFFMASK)) return -1;

//Dump_Data (pkt, plen);
	switch (iph->ip_p) {
		case IPPROTO_TCP:
			th = (struct tcphdr *)((const char *)iph + ihlen);
			thlen = th->th_off * 4;
			
			stat.tcp++;
			break;
		case IPPROTO_UDP:
			thlen = sizeof(struct udphdr);
			stat.udp++;
			break;
		case IPPROTO_ICMP:
			stat.icmp++;
			break;
		default:
			return -1;
	}
	p->mac.raw 	= pkt;
	p->nh.iph 	= iph;
	p->th.raw		= (char*)iph + ihlen;
	p->proto 		= iph->ip_p;

	offset = sizeof(struct ether_header) + ihlen + thlen;
	length = sizeof(struct ether_header) + ntohs(iph->ip_len) - offset;

	p->payload  		= pkt + offset;
	p->payload_len 	= length;

	return 0;
}

void view (mPacket_t *p)
{
	static unsigned int ti = 0, now;

	now = time (NULL);
	if (now > ti){
		ti = now + 5;

		fprintf (stderr, "\n[%5s][%5s] [%5s/%5s/%5s/%5s] [%5s]\n",
			"PKTS", "SIZE", "TCP", "UDP", "ICMP", "ETC", "DETEC");

		fprintf (stderr, "[%5llu][%5llu] [%5llu/%5llu/%5llu/%5llu] [%5llu]\n\n",
			stat.cnt, stat.size,
			stat.tcp, stat.udp, stat.icmp, stat.etc,
			stat.detec);
	}

	// proto sip sport - dip sport 
	switch (p->proto){
		case IPPROTO_TCP:
			fprintf (stderr, "%4s %15s %5d - %15s %5d\n",
					"TCP", 
					inet_ntoa (p->nh.iph->ip_src),
					ntohs (p->th.tcph->th_sport),
					inet_ntoa (p->nh.iph->ip_dst),
					ntohs (p->th.tcph->th_dport));
			break;
		case IPPROTO_UDP:
			fprintf (stderr, "%4s %15s %5d - %15s %5d\n",
					"UDP",
					inet_ntoa (p->nh.iph->ip_src),
					ntohs (p->th.udph->uh_sport),
					inet_ntoa (p->nh.iph->ip_dst),
					ntohs (p->th.udph->uh_dport));
			break;
		case IPPROTO_ICMP:
			fprintf (stderr, "%4s %15s %5s - %15s %5s\n",
					"ICMP",
					inet_ntoa (p->nh.iph->ip_src),
					" ",
					inet_ntoa (p->nh.iph->ip_dst),
					" ");
			break;
	}

	// payload ??
	return;
}

int search (mPacket_t *p)
{
	// pattern, "GET " & "html"
	char *pat1 = "GET ";
	int len1 = strlen (pat1);
	char *pat2 = "html";
	int len2 = strlen (pat2)

	if ( (p->payload_len < len1) || (p->payload_len <len2)) return 0;
	if ( (strstr (p->payload, pat1) == NULL) || (strstr (p->payload, pat2) == NULL) ) return 0;

	return 1;
}

int filter (mPacket_t *p)
{

	if (p->proto == IPPROTO_TCP ||
			(ntohs (p->th.tcph->th_sport) == 22 ||
			 ntohs (p->th.tcph->th_dport) == 22)) return 0;

	if (p->proto == IPPROTO_TCP ||
			(ntohs (p->th.tcph->th_sport) == 3389 ||
			 ntohs (p->th.tcph->th_dport) == 3389)) return 0;

	return 1;
}

void dissect (unsigned char *user, const struct pcap_pkthdr *h, const unsigned char *p)
{
	mPacket_t				pkt;
	memset (&pkt, 0, sizeof (mPacket_t));

	stat.cnt++;
	stat.size+=h->caplen;

	// decode
	if (decode (&pkt, p, (unsigned int)h->caplen)) return;

	// filter
	if (filter (&pkt)) return;

	// view
	view (&pkt);

	// detection
	if (search (&pkt)){
		fprintf (stderr, "\t\t---- Detection !! -----\n");
		Dump_data(p, (unsigned int)h->caplen);
	}
	return;
}

void io_read (char *dev)
{
	char 	*net;
	char 	*mask;
	int 	ret;
	char 	errbuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32 netp;  // ip
	bpf_u_int32 maskp; // submet mask
	struct in_addr addr;
	struct pcap_pkthdr header;  // Header that pcap gives us
	char *file = stdout;

	if (dev == NULL) return;

	pcap_t *pcd;
	pcd = pcap_open_live (dev, BUFSIZ, 1, 1000, errbuf);
	if (pcd == NULL) {
		fprintf (stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		return;	
	}

	fprintf (file, "===================================\n");
	fprintf (file, "Interface Info : %s\n", dev);

	ret = pcap_lookupnet (dev, &netp, &maskp, errbuf);
	if (ret == -1){
		fprintf (file,"%s\n", errbuf);
		return;
	}

	addr.s_addr = netp;
	net = inet_ntoa (addr);
	if (net == NULL){
		perror ("inet_ntoa");
		return;
	}
	fprintf (file,"NET : %s\n", net);

	addr.s_addr = maskp;
	mask = inet_ntoa (addr);
	if (mask == NULL){
		perror ("inet_ntoa");
		exit(1);
	}
	fprintf (file,"MASK : %s\n", mask);
	fprintf (file,"===================================\n");

	pcap_loop (pcd, 0, dissect, NULL);
	pcap_close (pcd);
	return;
}

int init ()
{
	memset (&stat, 0, sizeof (mStat_t));
	return 0;
}

int main (int argc, char **argv)
{
	char 	*dev = NULL;	
	int 	opt;

	enable_core ();
	while ((opt = getopt(argc, argv, "hi:d:")) != -1){
		switch(opt){
			case 'h':
				help();
				break;
			case 'i':
				dev = optarg;
				break;
			default:
				break;
		}
	}

	if (dev == NULL) { 
		fprintf(stderr, "Try `%s -h' for more information.\n", argv[0]);
		exit(0);
	} 

	init ();
	io_read (dev);
	return 0;
}



#include <iostream>

#include <stdint.h>  // For fixed-width integer types (u16, u32, etc.)
#include <cstring>
#include <unistd.h>

#include "main.hpp"
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>

using namespace std;

int sock; // icmp socket file descriptor

char *hostname;

int max_hops;

int main(int argc, char *argv[]) {
	int op;
	char *ip;
	int max_hops = 30;

	// process option
	opterr = 0;
	while ((op = getopt(argc, argv, "h:")) != EOF)
	{
		switch (op) {
			case 'h':
				max_hops = str2val(optarg, "max hops", 1, 255);
				break;
			default:
				usage();
		}
	}

	// process destination
	switch (argc - optind) {
		case 1:
			hostname = argv[optind];

			ip = gethostinfo(hostname);

			break;
		default:
			usage();
	}
	
	traceroute(ip, max_hops, 1); // initial ttl is 1

	return 0;
}

uint16_t checksum(void *b, int32_t len) {
    uint16_t *buf = (uint16_t *)b;  // Cast the void pointer to uint16_t*
    uint16_t oddbyte;
    uint32_t sum = 0;

    // Sum up the 16-bit words
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    // Add the remaining byte if the length is odd
    if (len == 1) {
		oddbyte = 0;
		*((u_char *) & oddbyte) = *(u_char *) buf;
        sum += oddbyte;  // Cast to uint8_t* to handle the last byte
    }

    // Fold the 32-bit sum into a 16-bit checksum
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);  // Handle carry
    return (uint16_t)(~sum);  // Return the one's complement
}

void traceroute(const char *ip, int max_hops, int respone_timeout) {
	fprintf(stdout, "over a maximum of %d hops:\n\n", max_hops);

	// Зберігаємо структуру місця призначення
	struct sockaddr_in to_addr;

	to_addr.sin_addr.s_addr = inet_addr(ip);	// Записуємо IP-адрес
	to_addr.sin_family = AF_INET;				// use ipv4
	to_addr.sin_port = rand();				// порт не потрібен для ICMP

	// Створення RAW-socket
	sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sock < 0) {
		perror("socket error");
		fprintf(stdout, "Run with sudo!");
		return;
	}

	struct icmphdr icmp_header;
	
	for (int i = 0; i < max_hops; i++) {
		// Заповнюємо ICMP заголовок
		icmp_header.type = ICMP_ECHO;
		icmp_header.code = 0;
		icmp_header.checksum = 0;
		icmp_header.un.echo.id = getpid();
		icmp_header.un.echo.sequence = i;
		// Розраховуємо контрольну суму
		icmp_header.checksum = checksum(&icmp_header, sizeof(struct icmphdr)); // TODO checksum

		// Встановлюємо TTL для пакета
		int ttl = i + 1;
		setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

		// Відправка пакета
		int send_flag = sendto(sock, &icmp_header, sizeof(icmp_header), 0,
			(struct sockaddr *) &to_addr, socklen_t(sizeof(to_addr)));
		if (send_flag < 0) {
			perror("send error");
			return;
		}

		// Отримуємо відповідь
		struct iphdr ip_response_header; // структура для збереження
	
		// Виставляємо тайм-аут очікування
		struct timeval tv;
		tv.tv_sec = respone_timeout;
		tv.tv_usec = 0;

		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

		// Очікуємо відповідь ICMP (recv)
		int data_length_bytes = recv(sock, &ip_response_header, sizeof(ip_response_header), 0);

		// Якщо таймаут
		if (data_length_bytes == -1) {
			fprintf(stdout, "%3d     *        *        *     Request timed out.\n", ttl);
			continue; // наступний hop
		}

		// Отримуємо ip-адрес відповіді
		struct sockaddr_in from_addr;
		struct in_addr sin_addr = from_addr.sin_addr;
		from_addr.sin_addr.s_addr = ip_response_header.saddr;

		// Спроба дізнатись ім'я
		string domain = "";
		
		hostent *hp = gethostbyaddr((void*) &sin_addr.s_addr, sizeof(sin_addr.s_addr), AF_INET);
		if (hp != NULL) {
			domain = "[" + string(hp->h_name) + "]";
		}

		fprintf(stdout, "%3d %5s %8s %8s     %s %s\n",
			ttl,
			"*",
			"*",
			"*",
			inet_ntoa(from_addr.sin_addr),
			domain.c_str());
		
		// Якщо отримали адрес цільового вузла. Завершити
		if (strcmp(inet_ntoa(sin_addr), ip) == 0) {
			fprintf(stdout, "\n%d hops between you and %s\n\n", ttl, ip);
			break;
		}
	}
	fprintf(stdout, "Trace complete.\n\n");
}

char* gethostinfo(char *hostname) {
	struct hostent *he;

    he = gethostbyname(hostname); // Запит в DNS для отримання ip

    if (he == nullptr) {
        herror("gethostbyname");

        cout << "1. Check your Network Connection. " << endl
        << "2. Check your DNS in /etc/resolv.conf - may be its unreachable" << endl;

        exit(1);
    }

	// Записати перший знайдений IP в рядок та повернути його
    char* ip = strdup(inet_ntoa(*(struct in_addr *)he->h_addr));;

	fprintf(stdout, "Tracing route to %s (%s)\n", he->h_name, ip);
	
    return ip;
}

int str2val(const char *str, const char *what, int min, int max) {
	int val = atoi(str);

	if (val < min && min >= 0) {
		if (min == 0)
			fprintf(stderr, "traceroute: %s must be >= %d\n", what, min);
		else
			fprintf(stderr, "traceroute: %s must be > %d\n", what, min - 1);
		exit(1);
	}
	if (val > max && max >= 0) {
		fprintf(stderr, "traceroute: %s must be <= %d\n", what, max);
		exit(1);
	}

	return val;
}

void usage() {
	fprintf(stderr,
		"Usage: traceroute [-h max_hops] host\n"
	);
	exit(1);
}
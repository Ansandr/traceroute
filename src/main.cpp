#include <iostream>

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

void traceroute(const char *ip, int max_hops, int respone_timeout) {

	// Зберігаємо структуру місця призначення
	struct sockaddr_in to_addr;

	to_addr.sin_addr.s_addr = inet_addr(ip);	// Записуємо IP-адрес
	to_addr.sin_family = AF_INET;				// use ipv4
	to_addr.sin_port = rand();				// порт не потрібен для ICMP

	// Створення RAW-socket
	sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sock < 0) {
		perror("socket error");
		return;
	}

	struct icmphdr icmp_header;
	
	for (int i = 0; i < max_hops; i++) {
		// Заповнюємо ICMP заголовок
		icmp_header.type = ICMP_ECHO;
		icmp_header.un.echo.id = getpid();
		icmp_header.un.echo.sequence = i;
		// Розраховуємо контрольну суму
		icmp_header.checksum = 0; // TODO checksum

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
			cout << ttl << "* * *" << endl;
			continue; // наступний hop
		}

		// Отримуємо ip-адрес відповіді
		struct sockaddr_in from_addr;
		from_addr.sin_addr.s_addr = ip_response_header.saddr;
		
		cout << ttl << " " << inet_ntoa(from_addr.sin_addr) << endl;
		
		// Якщо з відповіді, ip-адрес цільового вузла. Завершити
		if (strcmp(inet_ntoa(from_addr.sin_addr), ip)) {
			cout << endl << ttl << " hops between you and " << ip << endl;
			break;
		}
	}
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

    cout << "Hostname: " << he->h_name << endl;
    cout << "IP address: " << inet_ntoa(*(struct in_addr *) he->h_addr) << endl;

	// Записати перший знайдений IP в рядок та повернути його
    char ip[1024];
    strcpy(ip, inet_ntoa(*(struct in_addr *) he->h_addr));
    return ref(ip);
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
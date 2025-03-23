#include <iostream>

#include <string.h>
#include <unistd.h>

#include "main.hpp"
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

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
	
	return 0;
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
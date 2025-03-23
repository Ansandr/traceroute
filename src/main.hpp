void usage();
int str2val(const char *str, const char *what, int min, int max);
char* gethostinfo(char *hostname);
void traceroute(const char *ip, int max_hops, int respone_timeout);
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
	cout << "Netowrk Tracer starting...\n";
	cout << "Arguments: " << argc << endl;
	
	int i = 0;
	while (i < argc) {
		cout << "Argument (" << i << "): " << argv[i] << endl;
		i++;
	}

	return 0;
}
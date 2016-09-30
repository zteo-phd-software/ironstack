#include "ip_address.h"

int main(int argc, char** argv) {

	ip_address address;
	address.set("firefly01.cs.cornell.edu");
	printf("result is %s\n", address.to_string().c_str());

	return 0;
}

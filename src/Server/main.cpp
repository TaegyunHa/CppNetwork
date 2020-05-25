#include <cstdlib>
#include <cstring>
#include <iostream>
//#include <io.h>
//#include <unistd.h>

#include "../Common/Packet.h"
#include "Server.h"

bool hello()
{
	std::cout << "hello" << std::endl;
	return true;
}

int main()
{
	Server server(DEFAULT_PORT);

	std::function<bool()> testFn = hello;

	server.init(hello);

	std::cout << "Close\n";

	std::this_thread::sleep_for(std::chrono::seconds(10));

	return 0;
}

#include "../Common/Packet.h"
#include "../Common/SgtWinsock.h"
#include "Client.h"
#include <vector>

Client::Client()
{
	if (SgtWinsock::getInstance().isInit())
	{
		SOCKET ConnectSocket = INVALID_SOCKET;
		addrinfo *result = NULL;
		addrinfo *ptr    = NULL;
		addrinfo hints;
		const char *sendbuf = "this is a test2";
		char recvbuf[DEFAULT_BUFLEN];
		int iResult;
		int recvbuflen = DEFAULT_BUFLEN;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// Resolve the server address and port
		iResult = getaddrinfo(LOOPBACK_IPv4, DEFAULT_PORT, &hints, &result);
		if (iResult != 0) {
			printf("getaddrinfo failed with error: %d\n", iResult);
		}

		// Attempt to connect to an address until one succeeds
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

			// Create a SOCKET for connecting to server
			ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
				ptr->ai_protocol);
			if (ConnectSocket == INVALID_SOCKET) {
				printf("socket failed with error: %ld\n", WSAGetLastError());
			}

			// Connect to server.
			iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult == SOCKET_ERROR) {
				closesocket(ConnectSocket);
				ConnectSocket = INVALID_SOCKET;
				continue;
			}
			break;
		}

		freeaddrinfo(result);

		if (ConnectSocket == INVALID_SOCKET) {
			printf("Unable to connect to server!\n");
		}

		pkt::StringPacket customPkt;
		customPkt.header.size = sizeof(pkt::StringPacket);
		const char* string2Send = "Hello, World\n\0";
		strcpy_s(customPkt.data.firstString, strlen(string2Send) + 1, string2Send);

		for (int i = 0; i < 100000; ++i)
		{
			iResult = send(ConnectSocket, reinterpret_cast<const char*>(&customPkt), customPkt.header.size, 0);
		}


		// Send an initial buffer
		iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
		for (int i = 0; i < 100000; ++i)
		{
			iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
		}
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
		}

		printf("Bytes Sent: %ld\n", iResult);

		// shutdown the connection since no more data will be sent
		//iResult = shutdown(ConnectSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
		}

		// Receive until the peer closes the connection
		do {

			iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0)
				printf("Bytes received: %d\n", iResult);
			else if (iResult == 0)
				printf("Connection closed\n");
			else
				printf("recv failed with error: %d\n", WSAGetLastError());

		} while (iResult > 0);

		// cleanup
		closesocket(ConnectSocket);
	}
}
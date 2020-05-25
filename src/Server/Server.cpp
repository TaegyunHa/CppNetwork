
// std
#include <stdio.h>
#include <iostream>
#include <chrono>

#include "../Common/Packet.h"
#include "../Common/SgtWinsock.h"
#include "Server.h"

static constexpr auto ONE_SEC = std::chrono::milliseconds(1000);

Server::Server(const std::string& serviceName, const std::string& nodeName)
	: m_serviceName(serviceName), m_nodeName(nodeName),
	m_listenSocket(INVALID_SOCKET), m_clientScheme(ClientScheme::Multi)
{
}

Server::~Server()
{
	close();
}

void Server::close()
{
	std::lock_guard<std::mutex> guard(m_serverMtx);
	m_isRunning.store(false);
	// Stop accepting new connection to a client
	closesocket(m_listenSocket);
	if (m_acceptThread.joinable())
		m_acceptThread.join();

	// Shutdown all existing connections to clients
	m_pChildClientVec.clear();
}

void Server::init(std::function<bool()> pktRecvFn)
{
	std::lock_guard<std::mutex> guard(m_serverMtx);

	// Assigne Callback Function to process received packet
	m_pktRecvFn = pktRecvFn;

	if (SgtWinsock::getInstance().isInit())
	{
		m_isRunning.store(true);

		addrinfo *result = NULL;
		addrinfo *ptr = NULL;
		addrinfo hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;     // IPv4 address family. <AF_UNSPEC> for not care IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM; // specify the TCP stream socket
		hints.ai_protocol = IPPROTO_TCP; // specify the TCP protocol
		hints.ai_flags = AI_PASSIVE;  // fill in my IP for me (drop to specify ip addr)

		const char* test = get_nodeName();
		const char* test2 = get_serviceName();

		int iResult = getaddrinfo(
			get_nodeName(),          // <www.website.com> or <IP addr> to connect to. Assign the address of my local host with NULL.
			get_serviceName(),       // <http> or <port number>. port can be (1025 ~ 65535)
			&hints,
			&result);

		//int iResult = getaddrinfo(
		//	LOOPBACK_IPv4,          // <www.website.com> or <IP addr> to connect to. Assign the address of my local host with NULL.
		//	DEFAULT_PORT,       // <http> or <port number>. port can be (1025 ~ 65535)
		//	&hints,
		//	&result);

		if (iResult != 0)
		{
			printf("getaddrinfo failed: %d\n", iResult);
			return;
		}

		// Create a SOCKET for the server to listen for client connections
		m_listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

		// Check for errors to ensure that the socket is a valid socket
		if (m_listenSocket == INVALID_SOCKET)
		{
			printf("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			return;
		}

		/* For a server to accept client connections, it must be bound to a network address within the system.
		bind a socket that has already been created to an IP address and port
		Call the bind function, passing the created socket and sockaddr structure returned from the getaddrinfo function as parameters.	*/

		// Setup the TCP listening socket
		iResult = bind(m_listenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closesocket(m_listenSocket);
			return;
		}

		showAllIPAddr(result);

		/*Once the bind function is called, the address information returned by the getaddrinfo function is no longer needed.
		The freeaddrinfo function is called to free the memory allocated by the getaddrinfo function for this address information. */
		freeaddrinfo(result);

		/* Call the listen function, passing as parameters the created socket and a value for the backlog,
		maximum length of the queue of pending connections to accept. */
		if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			printf("Listen failed with error: %ld\n", WSAGetLastError());
			closesocket(m_listenSocket);
		}
		else
		{
			if (m_clientScheme == ClientScheme::Multi)
			{
				// Start a thread that accepting a new client
				startAcceptMultiClients();
			}
			else if (m_clientScheme == ClientScheme::Single)
			{
				startAcceptSingleClients();
			}
		}
	}
}

void Server::startAcceptMultiClients()
{
	// Set non-blocking accept
	u_long nonblocking_long = 1;	// blocking = 0, non-blocking = 1
	if (ioctlsocket(m_listenSocket, FIONBIO, &nonblocking_long))
	{
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(m_listenSocket);
	}

	/* This thread will search for a new client
		 * every 1 second and add it into <m_pChildClientVec>
		 * once it finds one */
	m_acceptThread = std::thread([this]()
	{
		while (m_isRunning.load())
		{
			// Create a temporary SOCKET object called ClientSocket for accepting connections from clients.
			SOCKET ClientSocket = INVALID_SOCKET;

			// Accept a client if exists with non-blocking scheme
			ClientSocket = accept(m_listenSocket, NULL, NULL);

			// Add a new child client if accepted socket is valid
			if (ClientSocket == INVALID_SOCKET)
			{
				printf("accept failed with error: %ld\n", WSAGetLastError());
			}
			else
			{
				std::lock_guard<std::mutex> guard(m_childClientMtx);
				m_pChildClientVec.emplace_back(std::make_unique<ChildClient>(this, ClientSocket));
			}

			if (m_isRunning.load())
				std::this_thread::sleep_for(ONE_SEC);
		}
	});
}

void Server::startAcceptSingleClients()
{
	// Set blocking receive scheme
	u_long nonblocking_long = 1;	// blocking = 0, non-blocking = 1
	if (ioctlsocket(m_listenSocket, FIONBIO, &nonblocking_long))
	{
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(m_listenSocket);
	}

	/* This thread will search for a new client
	and start recv/send process if it finds the client.
	Once the connection to the client closed, it will start
	searching for a new client again and connec to it if it finds it */
	m_acceptThread = std::thread([this]()
	{
		while (m_isRunning.load())
		{
			// Create a temporary SOCKET object called ClientSocket for accepting connections from clients.
			SOCKET ClientSocket;
			ClientSocket = INVALID_SOCKET;

			// Accept a client if exists with non-blocking scheme
			ClientSocket = accept(m_listenSocket, NULL, NULL);

			// Add a new child client if accepted socket is valid
			if (ClientSocket = !INVALID_SOCKET)
			{
				// this function has a do-while loop
				// and will last until connection closed
				recvSendProcess(ClientSocket);
			}

			if (m_isRunning.load())
				std::this_thread::sleep_for(ONE_SEC);
		}
	});
}

void Server::recvSendProcess(SOCKET ClientSocket)
{
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int bytesReceived;

	// Running recv and send until 
	do
	{
		// This will wait until a new packet comes in
		bytesReceived = recv(ClientSocket, recvbuf, recvbuflen, 0);

		if (bytesReceived > 0)
		{
			printf("Bytes received: %d\n", bytesReceived);
			recvbuf[min(bytesReceived, DEFAULT_BUFLEN - 1)] = '\0';
			printf("- %s\n", recvbuf);

			//// Echo the buffer back to the sender
			//iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			//if (iSendResult == SOCKET_ERROR) {
			//	printf("send failed: %d\n", WSAGetLastError());
			//	closesocket(ClientSocket);
			//}
			//printf("Bytes sent: %d\n", iSendResult);
		}
		else if (bytesReceived == 0)
		{
			printf("Connection closing...\n");
		}
		else
		{
			printf("recv failed: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
		}
	} while (m_isRunning.load() && bytesReceived > 0);
}

ChildClient::ChildClient(Server* parent, SOCKET ClientSocket)
	: m_isAlive(true), m_parent(parent), m_clientSocket(ClientSocket)
{
	// Set non-blocking accept
	u_long nonblocking_long = 0;	// blocking = 0, non-blocking = 1
	if (ioctlsocket(m_clientSocket, FIONBIO, &nonblocking_long))
	{
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(m_clientSocket);
	}

	m_recvThread = std::thread([this]()
	{
		int bytesReceived;
		
		// Receive until the peer shuts down the connection
		do {
			bytesReceived = receivePkt();

			// Receive header of pkt first
			// bytesReceived = recv(m_clientSocket, tmpRecvPktHeader.data(), pktHeaderSize, 0);

			if (bytesReceived > 0)
			{
				//const Packet* pktTemp = reinterpret_cast<Packet*>(tmpRecvPkt.data());

				//printf("Bytes received: %d\n", bytesReceived);
				//printf("- %s\n", recvbuf);
				//printf("- %s\n", pktTemp->testStr);

				//// Echo the buffer back to the sender
				//iSendResult = send(ClientSocket, recvbuf, iResult, 0);
				//if (iSendResult == SOCKET_ERROR) {
				//	printf("send failed: %d\n", WSAGetLastError());
				//	closesocket(ClientSocket);
				//}
				//printf("Bytes sent: %d\n", iSendResult);
			}
			else if (bytesReceived == 0)
			{
				printf("Connection closing...\n");
			}
			else
			{
				printf("recv failed: %d\n", WSAGetLastError());
				// closesocket(m_clientSocket);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(30));
		} while (m_isAlive.load()/* && bytesReceived > 0*/);
	});
}

int ChildClient::receivePkt()
{
	static pkt::Header pktHeader;
	static size_t pktHeaderSize = sizeof(pkt::Header);
	static std::vector<char> tmpRecvPkt(DEFAULT_BUFLEN);

	// initialize the value of header with 0
	memset(&pktHeader, 0, pktHeaderSize);

	// initialize the temporary packet header with 0
	std::fill(tmpRecvPkt.begin(), tmpRecvPkt.end(), 0);

	// Receive header of pkt first
	int bytesReceived = recv(m_clientSocket, tmpRecvPkt.data(), DEFAULT_BUFLEN, 0);

	if (bytesReceived > 0)
	{
		m_recvBuffer.insert(m_recvBuffer.end(), tmpRecvPkt.begin(), tmpRecvPkt.begin() + bytesReceived);

		std::vector<char> tmpHeader(m_recvBuffer.begin(), m_recvBuffer.begin() + sizeof(pkt::Header));

		const pkt::Header* pktTemp = reinterpret_cast<pkt::Header*>(tmpHeader.data());
		printf("Bytes received: %d\n", bytesReceived);
		printf("PktSize: %d\n", (int)pktTemp->size);
		
		// Get Pkt size to recv
		uint32_t pktSize2Recv = pktTemp->size;

		if (pktSize2Recv > 0)
		{

			std::vector<char> tmpPkt2Extract(m_recvBuffer.begin(), m_recvBuffer.begin() + pktSize2Recv);
		
			m_recvBuffer.erase(m_recvBuffer.begin(), m_recvBuffer.begin() + pktSize2Recv);

			pkt::StringPacket* newPkt = reinterpret_cast<pkt::StringPacket*>(tmpPkt2Extract.data());
			printf("Print first string: %s", newPkt->data.firstString);
		}

		//// Echo the buffer back to the sender
		//iSendResult = send(ClientSocket, recvbuf, iResult, 0);
		//if (iSendResult == SOCKET_ERROR) {
		//	printf("send failed: %d\n", WSAGetLastError());
		//	closesocket(ClientSocket);
		//}
		//printf("Bytes sent: %d\n", iSendResult);
	}

	return bytesReceived;
}

ChildClient::~ChildClient()
{
	/* When the server is done sending data to the client,
	the shutdown function can be called specifying SD_SEND to shutdown the sending side of the socket.
	This allows the client to release some of the resources for this socket.
	The server application can still receive data on the socket.*/

	// shutdown the send half of the connection since no more data will be sent
	int iResult = shutdown(m_clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(m_clientSocket);
	}

	/*When the client application is done receiving data, the closesocket function is called to close the socket.
	When the client application is completed using the Windows Sockets DLL, the WSACleanup function is called to release resources.*/
	// Cleanup Socket
	closesocket(m_clientSocket);

	// Cleanup thread
	m_isAlive.store(false);
	if (m_recvThread.joinable())
		m_recvThread.join();
}

void Server::showAllIPAddr(addrinfo* result)
{
	if (result == nullptr)
		return;

	char ipstr[INET6_ADDRSTRLEN];
	for (addrinfo* pRes = result; pRes != NULL; pRes = pRes->ai_next)
	{
		void* pAddr;
		std::string ipver;

		// Get the pointer to the address
		if (pRes->ai_family == AF_INET)
		{	// IPv4
			sockaddr_in* ipv4 = (sockaddr_in*)(pRes->ai_addr);
			pAddr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		else
		{
			// IPv6
			sockaddr_in6* ipv6 = (sockaddr_in6*)(pRes->ai_addr);
			pAddr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}

		// convert the IP to a string and print it:
		inet_ntop(pRes->ai_family, pAddr, ipstr, sizeof ipstr);
		printf("  %s: %s\n", ipver.c_str(), ipstr);
	}
}

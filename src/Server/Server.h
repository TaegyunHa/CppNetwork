#pragma once
#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include <condition_variable>

// Winsock
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

enum class ClientScheme
{
	Single,
	Multi
};

struct addrinfo;
class ChildClient;
class Server
{
public:
// Lifecycle
	Server(
		const std::string& serviceName = "",
		const std::string& nodeName = "");		// empty string for local loopback
	~Server();

	void close();
	void init(std::function<bool()> pktRecvFn);

	inline const char* get_nodeName() { return m_nodeName.empty() ? NULL : m_nodeName.c_str(); }
	inline const char* get_serviceName() { return m_serviceName.empty() ? NULL : m_serviceName.c_str(); }

private:
	void startAcceptMultiClients();
	void startAcceptSingleClients();
	void recvSendProcess(SOCKET ClientSocket);
	void showAllIPAddr(addrinfo* result);

	const std::string m_serviceName;	// <http> or <port number>
	const std::string m_nodeName;		// <www.something.com> or <IP Addr>

	// Connection Status
	std::atomic<bool> m_isRunning;
	std::mutex m_serverMtx;
	std::condition_variable m_statusCv;

	// Socket
	SOCKET m_listenSocket;
	std::thread m_acceptThread;
	std::mutex m_childClientMtx;
	std::vector<std::unique_ptr<ChildClient>> m_pChildClientVec;
	
	// Function to process the packet
	std::function<bool()> m_pktRecvFn;

	ClientScheme m_clientScheme;

};


class ChildClient
{
public:
	ChildClient(Server* parent, SOCKET ClientSocket);
	~ChildClient();

	int receivePkt();

private:
	std::atomic<bool> m_isAlive;
	const Server* m_parent;
	const SOCKET m_clientSocket;
	std::thread m_recvThread;

	std::vector<char> m_recvBuffer;

};

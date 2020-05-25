/* This will manage lifetime of winsock with
 * Meyers Singleton pattern.
 * This class will do:
 *	- call <WSAStartup(...)> before the socket programing
 *	- call <WSACleanup()> before terminate the program */

#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <atomic>
#pragma comment(lib, "Ws2_32.lib")
class SgtWinsock
{
private:
	// Singleton Design Pattern
	SgtWinsock();
	~SgtWinsock();
	SgtWinsock(const SgtWinsock&) = delete;
	SgtWinsock& operator=(const SgtWinsock&) = delete;

	// Operation
	bool init();

	WSADATA m_wsaData;
	std::atomic<bool> m_isInit = false;
	std::atomic<bool> m_isRestarting = false;

public:
	// Singleton Design Pattern
	static SgtWinsock& getInstance()
	{
		static SgtWinsock instance;
		return instance;
	}

	// ACCESS
	inline bool isInit() { return m_isInit.load(); }
	
	// Operation
	void WSARestart();

};

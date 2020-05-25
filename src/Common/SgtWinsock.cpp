#include "SgtWinsock.h"

SgtWinsock::SgtWinsock()
	: m_isInit(false), m_isRestarting(false)
{
	init();
}

SgtWinsock::~SgtWinsock()
{
	int wsRet = WSACleanup();
	if (wsRet != 0)
		printf("WSAStartup failed: %d\n", wsRet);
}

bool SgtWinsock::init()
{
	// Using Winsock v2.2
	int wsRet = WSAStartup(MAKEWORD(2, 2), &m_wsaData);
	if (wsRet != 0)
	{
		printf("WSAStartup failed: %d\n", wsRet);
		return false;
	}
	else
	{
		m_isInit.store(true);
		return true;
	}
}

void SgtWinsock::WSARestart()
{
	/* Code below will be accessed once at a time
	 * for thread safe impl	*/
	if (!m_isInit.load() && !m_isRestarting.exchange(true))
	{
		// Initialize Winsock
		init();

		m_isRestarting.store(false);
	}
}

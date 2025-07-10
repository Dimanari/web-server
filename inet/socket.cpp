#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")
#else
#include <netinet/tcp.h> /* TCP_NODELAY */
#endif

#include <stdio.h> /* sprintf */
#include <string.h> /* memset */
#include <stdlib.h> /* exit */
#include "socket.hpp"

namespace dimanari
{
#ifdef _WIN32
// fucking Windows
#define SHUT_RDWR SD_BOTH
static int close(SOCKET s)
{
	return closesocket(s);
}
#else
#endif

	Socket::Socket(int port, int family, int protocol, int type) : m_connected(false), m_listening(false)
	{
		OpenListener(port, family, protocol, type);
	}


	Socket::Socket(const char* addr, int port, int family, int protocol, int type) : m_connected(false), m_listening(false)
	{
		OpenComm(addr, port, family, protocol, type);
	}

	int Socket::OpenListener(int port, int family, int protocol, int type)
	{
		struct addrinfo* hints;
		SetInfo(NULL, port, family, protocol, type, &hints);
		m_sock = socket(hints->ai_family, hints->ai_socktype, hints->ai_protocol);
		if (INVALID_SOCKET == m_sock)
		{
#ifdef _WIN32
			printf("socket() failed with error: %d\n", WSAGetLastError());
#else
			perror("socket() failed with error:");
#endif
			return 1;
		}
		socklen_t ena = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&ena, sizeof(ena)); //Make it possible to re-bind to a port that was used within the last 2 minutes
		// setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&ena, sizeof(ena)); //Used for interactive TCP programs

		Bind(hints);
		freeaddrinfo(hints);
		return 0;
	}

	int Socket::OpenComm(const char* addr, int port, int family, int protocol, int type)
	{
		struct addrinfo* hints;
		int iResult;
		SetInfo(addr, port, family, protocol, type, &hints);
		struct addrinfo* ptr;
		for (ptr = hints; ptr != NULL; ptr = ptr->ai_next)
		{
			// Create a SOCKET for connecting to server
			m_sock = socket(ptr->ai_family, ptr->ai_socktype,
				ptr->ai_protocol);
			if (m_sock == INVALID_SOCKET) {
				return 2;
			}

			// Connect to server.
			iResult = connect(m_sock, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult == SOCKET_ERROR) {

#ifdef _WIN32
				printf("connect() failed with error: %d\n", WSAGetLastError());
#else
				perror("connect() failed with error:");
#endif
				Close();
				//continue;
			}
			break;
		}
		/*
		m_sock = socket(hints->ai_family, hints->ai_socktype, hints->ai_protocol);
		Connect(hints);
		*/
		freeaddrinfo(hints);
		return INVALID_SOCKET == m_sock;
	}

	int Socket::OpenUDP(int family, int protocol, int type)
	{
		m_sock = socket(family, type, protocol);
		if (m_sock == INVALID_SOCKET) {
			return 2;
		}
		return INVALID_SOCKET == m_sock;
	}

	void Socket::SetInfo(const char* str, int port, int family, int protocol, int type, struct addrinfo** hints, int flags)
	{
		struct addrinfo info;
		memset(&info, 0, sizeof(info));
		info.ai_family = family;
		info.ai_protocol = protocol;
		info.ai_socktype = type;
		info.ai_flags = flags;

		if (NULL == str)
		{
			info.ai_flags |= AI_PASSIVE;
		}

		GetAddrs(str, port, &info, hints);
	}
	void Socket::AddFd(fd_set* _fds)
	{
		FD_SET(m_sock, _fds);
	}
	int Socket::Bind(struct addrinfo* hints)
	{
		int success = bind(m_sock, hints->ai_addr, hints->ai_addrlen);
		if (0 != success)
		{
#ifdef _WIN32
			printf("bind() failed with error: %d\n", WSAGetLastError());
#else
			perror("bind() failed with error:");
#endif
			//exit(1);
		}
		return success;
	}

	int Socket::Connect(struct addrinfo* hints)
	{
		int success = connect(m_sock, hints->ai_addr, hints->ai_addrlen);
		if (SOCKET_ERROR == success)
		{
#ifdef _WIN32
			printf("connect() failed with error: %d\n", WSAGetLastError());
#else
			perror("connect() failed with error:");
#endif
			return success;
		}
		m_connected = true;
		return success;
	}

	Socket::Socket() : m_sock(INVALID_SOCKET), m_connected(false), m_listening(false)
	{
	}

	Socket::~Socket()
	{
	}

	Socket Socket::Accept(bool no_action)
	{
		if(Listen() || no_action)
		{
			return Socket();
		}
		Socket new_sock;

		sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		new_sock.m_sock = accept(m_sock, &addr, &addrlen);
		if (INVALID_SOCKET == new_sock.m_sock)
		{
#ifdef _WIN32
			printf("accept() failed with error: %d\n", WSAGetLastError());
#else
			perror("accept() failed with error:");
#endif
		}
		else
			new_sock.m_connected = true;

		return new_sock;
	}

	int Socket::Listen()
	{
		if (!m_listening)
		{
			if (SOCKET_ERROR == listen(m_sock, SOMAXCONN))
			{
#ifdef _WIN32
				printf("listen() failed with error: %d\n", WSAGetLastError());
#else
				perror("listen() failed with error:");
#endif
				return 1;
			}

			m_listening = true;
		}
		return 0;
	}

	int Socket::Send(const char* message, size_t size, int falgs)
	{
		return send(m_sock, message, size, falgs);
	}
	int Socket::Recv(char* buffer, size_t size, int falgs)
	{
		return recv(m_sock, buffer, size, falgs);
	}

	int Socket::SendTo(const char* message, size_t size, const sockaddr* recipient, socklen_t recipient_len, int falgs)
	{
		return sendto(m_sock, message, size, falgs, recipient, recipient_len);
	}

	int Socket::RecvFrom(char* buffer, size_t size, sockaddr* recipient, socklen_t* recipient_len, int falgs)
	{
		return recvfrom(m_sock, buffer, size, falgs, recipient, recipient_len);
	}

	void Socket::Halt()
	{
		if (INVALID_SOCKET != m_sock)
		{
			shutdown(m_sock, SHUT_RDWR);
			m_sock = -1;
		}
	}

	void Socket::Close()
	{
		if (INVALID_SOCKET != m_sock)
		{
			close(m_sock);
			m_sock = INVALID_SOCKET;
		}
	}

	int NetInit()
	{
#ifdef _WIN32
		WSAData wsa_data;
		int iResult = 0;
		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
		if (iResult != 0)
		{
			return iResult;
		}
#endif
		return 0;
	}

	int NetClose()
	{
#ifdef _WIN32
		return WSACleanup();
#else
		return 0;
#endif
	}

	void GetAddrs(const char* hostname, int port, const struct addrinfo* info, struct addrinfo** hints)
	{
		char str[20];
		sprintf_s(str, sizeof(str), "%d", port);
		
		int iResult = getaddrinfo(hostname, str, info, hints);
		if (iResult != 0) {
			printf("getaddrinfo failed with error: %d\n", iResult);
		}
	}
}
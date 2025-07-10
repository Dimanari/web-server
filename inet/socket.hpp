#pragma once
#ifndef DIMA_SOCKET_HPP
#define DIMA_SOCKET_HPP
/*
 * Simple Socket Interface by Dimanari(Daniell Bar)
 * this is a modular interface that allows for easy creation
 * and deployment of Socket based IPC.
 *
 * Please don't delete that segment.
 */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <netdb.h> /* GetHostbyname */
#include <sys/socket.h> /* socket */
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define CONNECTION_CLOSED 0
#define Sleep(x) usleep(1000 * x)
#endif
struct addrinfo;

namespace dimanari
{
	int NetInit();// windows compatability call
	int NetClose();// windows compatability call
	class Socket
	{
	public:
		Socket(int port, int family, int protocol, int type);
		Socket(const char* addr, int port, int family, int protocol, int type);
		int OpenListener(int port, int family, int protocol, int type);
		int OpenComm(const char* addr, int port, int family, int protocol, int type);
		int OpenUDP(int family, int protocol, int type);

		// send&recv used in TCP
		int Send(const char* message, size_t size, int falgs = 0);
		int Recv(char* buffer, size_t size, int falgs = 0);

		// sendto&recvform used in UDP
		int SendTo(const char* message, size_t size, const sockaddr* recipient, socklen_t recipient_len, int falgs = 0);
		int RecvFrom(char* buffer, size_t size, sockaddr* recipient, socklen_t* recipient_len, int falgs = 0);

		//sockaddr_storage last_sender;
		//socklen_t last_size;
		~Socket();
		Socket Accept(bool no_action = false);
		int Listen();
		Socket(); /* Invalid Constructor */
		inline bool IsInvalid();
		void Halt();
		void Close();
		static void SetInfo(const char* str, int port, int family, int protocol, int type, struct addrinfo** hints, int flags = 0);
		void AddFd(fd_set *_fds);
	protected:
		inline SOCKET GetSock();
		inline void SetSock(SOCKET sock);
		int Bind(struct addrinfo* hints);
		int Connect(struct addrinfo* hints);
	private:
		friend class Server_Selector;
		SOCKET m_sock;
		bool m_connected;
		bool m_listening;
	};

	void GetAddrs(const char* hostname, int port, const struct addrinfo* info, struct addrinfo** hints);

	inline bool Socket::IsInvalid()
	{
		return INVALID_SOCKET == m_sock;
	}
	inline SOCKET Socket::GetSock()
	{
		return m_sock;
	}
	inline void Socket::SetSock(SOCKET sock)
	{
		Halt();
		Close();
		m_sock = sock;
	}
}
#endif //!DIMA_SOCKET_HPP
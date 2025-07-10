#include <inet/http_parser.hpp>
#include <database/query.hpp>
#include "web_server.hpp"
namespace dimanari
{
	void RunServerThreadFunc(HTTPServer* that, int port)
	{
		that->RunServer(port);
	}

	void HTTPServer::StartServerThread(int port)
	{
		m_thread = std::thread(RunServerThreadFunc, this, port);
	}

	void HTTPServer::StopServerThread()
	{
		m_keep_running.store(false);
		m_thread.join();
	}

	void HTTPServer::RunServer(int port)
	{
		printf("Starting Server\n");
		NetInit();
		fd_set read_fds = {};
		m_server_socket = Socket(port, AF_INET, IPPROTO_TCP, SOCK_STREAM);
		m_server_socket.Listen();
		m_keep_running = (true);
		while (m_keep_running.load())
		{
			FD_ZERO(&read_fds);
			m_server_socket.AddFd(&read_fds);
			timeval time;
			time.tv_usec = 0;
			time.tv_sec = 1;
			int error = select(1, &read_fds, nullptr, nullptr, &time);
			if (1 == error)
			{
				Socket recieve = m_server_socket.Accept();
				if (recieve.IsInvalid())
					continue;

				int read_amount = recieve.Recv(m_buffer, sizeof(m_buffer));

				if (0 == read_amount)
				{
					recieve.Close();
					continue;
				}

				message_class* msg_get = GenMessage(read_amount, m_buffer);
				message_class* msg_return;

				HttpParser::Parse(msg_get, &msg_return);


				if (msg_return)
				{
					recieve.Send(msg_return->data, msg_return->data_size);
					delete msg_return;
				}
				msg_return = nullptr;
				delete msg_get;
				msg_get = nullptr;
				recieve.Close();
			}
			else
			{
				switch (error)
				{
				case WSANOTINITIALISED:
					printf("A successful WSAStartup call must occur before using this function.\n");
					break;
				case WSAEFAULT:
					printf("The Windows Sockets implementation was unable to allocate needed resources for its internal operations, \nor the readfds, writefds, exceptfds, or timeval parameters are not part of the user address space.\n");
					break;
				case WSAENETDOWN:
					printf("The network subsystem has failed.\n");
					break;
				case WSAEINVAL:
					printf("The time-out value is not valid, or all three descriptor parameters were null.\n");
					break;
				case WSAEINTR:
					printf("A blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall.\n");
					break;
				case WSAEINPROGRESS:
					printf("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.\n");
					break;
				case WSAENOTSOCK:
					printf("One of the descriptor sets contains an entry that is not a socket.\n");
					break;
				}
				extern const char* db_path;
				QueryParse::SaveDB(db_path);
			}
		}

		m_server_socket.Close();
		NetClose();
	}
}
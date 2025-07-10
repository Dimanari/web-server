#include <inet/socket.hpp>
#include <atomic>
#include <thread>
#define BUFFER_SIZE 4096
namespace dimanari
{
	class HTTPServer
	{
	private:
		Socket m_server_socket;
		volatile std::atomic_bool m_keep_running;
		std::thread m_thread;
		char m_buffer[BUFFER_SIZE];
		int m_port;
	public:

		void StartServerThread(int port = 80);
		void StopServerThread();
		void RunServer(int port = 80);
	};

	void RunServerThreadFunc(HTTPServer* that, int port = 80);
}
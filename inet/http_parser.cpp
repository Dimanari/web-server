#include <sstream>
#include <iostream>
#include "web_parser.hpp"
#include "http_parser.hpp"
namespace dimanari
{

	void cleanurl(char* str)
	{
		char asciinum[3] = { 0 };
		int i = 0, c;

		while (str[i]) {
			if (str[i] == '+')
				str[i] = ' ';
			else if (str[i] == '%') {
				asciinum[0] = str[i + 1];
				asciinum[1] = str[i + 2];
				str[i] = strtol(asciinum, NULL, 16);
				c = i + 1;
				do {
					str[c] = str[c + 2];
				} while (str[2 + (c++)]);
			}
			++i;
		}
	}

	void cleanurl(std::string& str)
	{
		char asciinum[3] = { 0 };
		int i = 0, c;

		while (str.size() > i) {
			if (str[i] == '+')
				str[i] = ' ';
			else if (str[i] == '%') {
				asciinum[0] = str[i + 1];
				asciinum[1] = str[i + 2];
				str[i] = strtol(asciinum, NULL, 16);
				c = i + 1;
				do {
					str[c] = str[c + 2];
				} while (str[2 + (c++)]);
			}
			++i;
		}
	}

	message_class::message_class() : data_size(0)
	{
	}

	void* message_class::operator new(std::size_t count, size_t message_arg)
	{
		void* data = new char[sizeof(message_class) + message_arg];
		reinterpret_cast<message_class*>(data)->max_size = message_arg;
		return data;
	}

	void message_class::operator delete(void* self)
	{
		delete self;
	}

	message_class* GenMessage(int message_size, const char* const message_value)
	{
		message_class* mesage = new(message_size + 1) message_class;
		mesage->data_size = message_size;
		memset(mesage->data, 0, message_size + 1);
		memcpy(mesage->data, message_value, message_size);
		return mesage;
	}

	void Cookies(std::string& msg, std::vector<Cookie> cookies)
	{
		for (auto& cookie : cookies)
		{
			std::string cookie_string = "";

			msg += "Set-Cookie: ";
			msg += cookie.name;
			msg += "=";
			msg += cookie.value;
			switch (cookie.status)
			{
			case CS_REFRESH:
				msg += "; Max-Age=1800\r\n";
				break;
			default:
				msg += "; Max-Age=0\r\n";
				break;
			}

			msg += cookie_string;
		}
	}

	void AddCookie(std::vector<Cookie>& cookies, Cookie cookie)
	{
		bool add_new_cookie = true;
		for (auto& cc : cookies)
		{
			if (cc.name == cookie.name)
			{
				add_new_cookie = false;
				cc = cookie;
			}
		}
		if (add_new_cookie)
		{
			cookies.push_back(cookie);
		}
	}

	int HttpParser::Parse(message_class* mesage, message_class** reply)
	{
		std::istringstream str(mesage->data);
		int returnval = 0;
		std::string line;
		std::vector<std::string> lines;
		std::vector<Cookie> cookies;
		std::string line1;
		std::getline(str, line1);
		lines.push_back(line1);
		while (std::getline(str, line)) {
			lines.push_back(line);
			if (line.find("Connection:") != std::string::npos)
			{
				if ((line.find("keep-alive") != std::string::npos))
				{
					returnval = 1;
				}
				if ((line.find("Cookie") != std::string::npos))
				{
					Cookie cookie;
					cookie.status = CS_REFRESH;
					std::string acc;
					int stage = 0;
					for (auto a : line)
					{
						switch (stage)
						{
						case 0:
							if (a == ':')
								stage = 1;
							break;
						case 1:
							acc = "";
							if (a != ' ')
								stage = 2;
							break;
						case 2:
							if (a == '=')
							{
								stage = 3;
								cookie.name = acc;
								acc = "";
							}
							else
								acc += a;
							break;
						case 3:
							if (a == '\r')
							{
								stage = 4;
								cookie.value = acc;

								cookies.push_back(cookie);
							}
							else if (a == ';')
							{
								stage = 1;
								cookie.value = acc;

								cookies.push_back(cookie);
							}
							else
								acc += a;
							break;
						}
					}
				}
			}
		}

		std::string type, path;

		std::istringstream command(line1.c_str());
		command >> type >> path;
		cleanurl(path);

		std::string msg;

		if (type == "GET")
		{
			std::vector<std::string> data;

			Get(msg, path, type, cookies);
		}
		else if (type == "POST")
		{
			int i;
			for (i = 0; i < lines.size(); i++)
				if (lines[i].size() < 2)
					break;
			std::string postmsg = "";
			for (i++; i < lines.size(); i++)
			{
				for (int k = 0; k < lines[i].size(); ++k)
					postmsg += lines[i][k];
			}
			std::vector<std::string> data;
			std::string filemsg;
			split3(postmsg, data, '&');

			//simple WEB servers don't have a proper way to handle the data of a post messages and instead treat it the same way they would a GET.
			Post(msg, path, type, cookies, data);
		}
		else if (type == "HEAD")
		{
			Head(msg, path, type);
		}
		else if (type == "OPTIONS")
		{
			Options(msg, path, type);
		}
		else
		{
			BadReq(msg, path, type);
		}

		*reply = GenMessage(msg.size(), msg.c_str());

		return returnval;
	}

	int HttpParser::fileType(std::string str)
	{
		if (str.find("htm") != std::string::npos)
			return 0;
		if (str.find("jpg") != std::string::npos || str.find("jpeg") != std::string::npos)
			return 1;
		if (str.find("gif") != std::string::npos)
			return 2;
		if (str.find("css") != std::string::npos)
			return 3;
		if (str.find("mweb") != std::string::npos)
			return 4;
		//css
		return -1;
	}

	void HttpParser::Head(std::string& msg, std::string path, std::string type)
	{
		std::vector<std::string> path_sub;
		split3(path, path_sub);
		std::string filemsg;
		//try to locate the file
		FILE* pFile = 0;
		int errorNo = (fopen_s(&pFile, path_sub[0].c_str(), "rb"));
		//if file found send file
		long long sizeOfFile = 0;
		if (errorNo == 0)
		{
			msg = "HTTP/1.1 200 OK\n";
			while (path_sub.size() < 2)
			{
				path_sub.push_back("");
			}
			if (path_sub[0].size() < 2)
				path_sub[0] = "/index.htm";

			path_sub[0] = "www" + path_sub[0];
			int _type;
			std::vector<std::string> path_sub2;
			split3(path_sub[0], path_sub2, '.');
			msg += "Content-Type: ";
			switch (_type = fileType(path_sub2[path_sub2.size() - 1]))
			{
			case 0:
			case 4:
				msg += "text/html \r\n";
				break;
			case 1:
				msg += "image/jpeg \r\n";
				break;
			case 2:
				msg += "image/gif \r\n";
				break;
			case 3:
				msg += "text/css \r\n";
				break;
			default:
				msg += path_sub2[path_sub2.size() - 1];
				msg += " \r\n";
			}
			int c;
			while ((c = fgetc(pFile)) != EOF) { // standard C I/O file reading loop
				char _c = c;
				filemsg += _c;
				sizeOfFile++;
			}
			fclose(pFile);
			if (filemsg.size() != sizeOfFile)
				std::cout << "File reading caused corruption in the string" << std::endl;
		}
		//if file not found
		else
		{
			msg = "HTTP/1.1 404 Not Found\n";
			while (path_sub.size() < 2)
			{
				path_sub.push_back("");
			}
			if (path_sub[0].size() < 2)
				path_sub[0] = "/index.htm";

			path_sub[0] = "www" + path_sub[0];
			int _type;
			msg += "Content-Type: ";
			msg += "text/html \r\n";
			filemsg += "<html><head><title>Missing File</title></head><body>";
			filemsg += "Request Type: ";
			filemsg += type;
			filemsg += "<br>Path: ";
			filemsg += path_sub[0];
			filemsg += "<br>subdata: ";
			filemsg += path_sub[1];
			filemsg += "</body></html>";
		}
		msg += "Content-Length: ";
		if (sizeOfFile > 0)
			msg += std::to_string(sizeOfFile);
		else
			msg += std::to_string(filemsg.size());
		msg += "\r\n";

		//end header
		msg += "\r\n";
	}

	void HttpParser::BadReq(std::string& msg, std::string path, std::string type)
	{
		std::vector<std::string> path_sub;
		split3(path, path_sub);
		std::string filemsg;
		//try to locate the file
		FILE* pFile = 0;
		int errorNo = (fopen_s(&pFile, path_sub[0].c_str(), "rb"));
		//if file found send file
		long long sizeOfFile = 0;
		msg = "HTTP/1.1 400 Bad Request\n";
		while (path_sub.size() < 2)
		{
			path_sub.push_back("");
		}
		if (path_sub[0].size() < 2)
			path_sub[0] = "/index.htm";

		path_sub[0] = "www" + path_sub[0];
		int _type;
		msg += "Content-Type: ";
		msg += "text/html \r\n";
		filemsg += "<html><head><title>Missing File</title></head><body>";
		filemsg += "Request Type: ";
		filemsg += type;
		filemsg += "<br>Path: ";
		filemsg += path_sub[0];
		filemsg += "<br>subdata: ";
		filemsg += path_sub[1];
		filemsg += "</body></html>";
		msg += "Content-Length: ";
		if (sizeOfFile > 0)
			msg += std::to_string(sizeOfFile);
		else
			msg += std::to_string(filemsg.size());
		msg += "\r\n";

		//end header
		msg += "\r\n";
		for (int i = 0; i < filemsg.size(); i++)
			msg += filemsg.c_str()[i];
	}

	void HttpParser::Options(std::string& msg, std::string path, std::string type)
	{
		std::vector<std::string> path_sub;
		split3(path, path_sub);
		std::string filemsg;
		msg = "HTTP/1.1 204 No Content\n";
		msg += "Allow : OPTIONS, GET, HEAD, POST\n";
		msg += "\r\n";
	}

	void HttpParser::Get(std::string& msg, std::string path, std::string type, std::vector<Cookie>& cookies)
	{
		Post(msg, path, type, cookies, std::vector<std::string>());
	}

	void HttpParser::Post(std::string& msg, std::string path, std::string type, std::vector<Cookie>& cookies, std::vector<std::string> data)
	{
		std::vector<std::string> path_sub;
		split3(path, path_sub);
		std::string filemsg;
		//try to locate the file
		FILE* pFile = 0;

		while (path_sub.size() < 2)//path and arguments are needed
		{
			path_sub.push_back("");//if doesn't exist add an empty string
		}
		if (path_sub[0].size() < 2) // valid paths always starts with / therefore the smallest viable path size is 2
			path_sub[0] = "/index.htm"; // default path

		if (path_sub[1].size())
		{
			// process get url
			std::vector<std::string> post_url;
			split3(path_sub[1], post_url, '&');

			for (auto& a : post_url)
				data.push_back(a);
		}

		path_sub[0] = "www" + path_sub[0]; //default directory(avoid people messing with your server software)

		int errorNo = (fopen_s(&pFile, path_sub[0].c_str(), "rb")); //read binary so images and other files can be loaded as well
		//if file found send file
		long long sizeOfFile = 0;
		if (errorNo == 0)
		{
			msg = "HTTP/1.1 200 OK\n";

			int _type;
			std::vector<std::string> path_sub2;
			split3(path_sub[0], path_sub2, '.');//split to get the file suffix
			msg += "Content-Type: ";
			switch (_type = fileType(path_sub2[path_sub2.size() - 1]))
			{
			case 0:
			case 4:
				msg += "text/html \r\n";
				break;
			case 1:
				msg += "image/jpeg \r\n";
				break;
			case 2:
				msg += "image/gif \r\n";
				break;
			case 3:
				msg += "text/css \r\n";
				break;
			default:
				msg += path_sub2[path_sub2.size() - 1]; // just state the file suffix in case you are clueless and the client isn't
				msg += " \r\n";
			}
			int c;
			while ((c = fgetc(pFile)) != EOF) {
				char _c = c; // cast to char(could've been done in the assignment but it doesn't matter)
				filemsg += _c;
				sizeOfFile++;
			}
			fclose(pFile);
			if (filemsg.size() != sizeOfFile)
				std::cout << "File reading caused corruption in the string" << std::endl;

			if (4 == _type)
			{
				sizeOfFile = WebParser::Parse(filemsg, cookies, data);
			}
		}
		//if file not found
		else
		{
			msg = "HTTP/1.1 404 Not Found\n";
			while (path_sub.size() < 2)
			{
				path_sub.push_back("");
			}
			if (path_sub[0].size() < 2)
				path_sub[0] = "/index.htm";

			//path_sub[0] = "www" + path_sub[0];
			int _type;
			msg += "Content-Type: ";
			msg += "text/html \r\n";
			filemsg += "<html><head><title>Missing File</title></head><body>";
			filemsg += "Request Type: ";
			filemsg += type;
			filemsg += "<br>Path: ";
			filemsg += path_sub[0];
			filemsg += "<br>subdata: ";
			filemsg += path_sub[1];
			filemsg += "</body></html>";
		}
		Cookies(msg, cookies);
		msg += "Content-Length: ";
		if (sizeOfFile > 0)
			msg += std::to_string(sizeOfFile);
		else
			msg += std::to_string(filemsg.size());
		msg += "\r\n";

		//end header
		msg += "\r\n";
		for (int i = 0; i < filemsg.size(); i++)
			msg += filemsg.c_str()[i];
	}
}
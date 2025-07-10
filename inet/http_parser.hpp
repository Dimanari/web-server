#pragma once
#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP
#include "common.hpp"
namespace dimanari
{
	void cleanurl(char* str);
	void cleanurl(std::string& str);

	struct message_class
	{
		size_t max_size;
		size_t data_size;
		char data[0];
		message_class();
		void* operator new (size_t count, size_t message_arg);
		void operator delete  (void* self);
	};

	class HttpParser
	{
	public:
		static int Parse(message_class* mesage, message_class** reply);
		static int fileType(std::string str);
		static void Head(std::string& msg, std::string path, std::string type);
		static void BadReq(std::string& msg, std::string path, std::string type);
		static void Options(std::string& msg, std::string path, std::string type);
		//static int isSpecial(std::string path);

		static void Get(std::string& msg, std::string path, std::string type, std::vector<Cookie>& cookies);
		static void Post(std::string& msg, std::string path, std::string type, std::vector<Cookie>& cookies, std::vector<std::string> data);
	};

	template <class Container>
	void split3(const std::string& str, Container& cont,
		char delim = '?')
	{
		size_t current, previous = 0;
		current = str.find(delim);
		while (current != std::string::npos) {
			cont.push_back(str.substr(previous, current - previous));
			previous = current + 1;
			current = str.find(delim, previous);
		}
		cont.push_back(str.substr(previous, current - previous));
	}
	message_class* GenMessage(int message_size, const char* const message_value);
}
#endif//HTTP_PARSER_HPP
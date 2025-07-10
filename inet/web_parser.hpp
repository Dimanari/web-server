#pragma once
#include "common.hpp"
namespace dimanari
{
	class WebParser
	{
	public:
		static int Parse(std::string& file_content, std::vector<Cookie>& cookies, std::vector<std::string> data);
		static std::string Execute(std::string CodeContent, std::vector<Cookie>& cookies, std::vector<std::string> data);
	};
}
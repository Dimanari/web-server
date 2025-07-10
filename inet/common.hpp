#pragma once
#ifndef WEB_COMMON_HPP
#define WEB_COMMON_HPP
#include <string>
#include <vector>
namespace dimanari
{
	enum CookieStatus
	{
		CS_REFRESH, CS_REMOVE, CS_STATUS_MAX
	};

	struct Cookie
	{
		std::string name;
		std::string value;
		CookieStatus status;
	};
}
#endif//WEB_COMMON_HPP
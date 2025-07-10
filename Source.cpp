#include <stdio.h>
#include <string>
#include <code/web_server.hpp>
#include <database/query.hpp>
using namespace dimanari;

void GetLine(std::string& str)
{
	int c;
	do {
		c = getchar();
		if ('\r' == c || '\n' == c)
			continue;
		str += char(c);
	} while (c != '\n');
}
const char* db_path = "db/database.dbf";
int main()
{
	QueryParse::LoadDB(db_path);
	HTTPServer server;
	server.StartServerThread();
	std::string command;
	std::string current_database = "default_db";
	bool keep_going = true;

	while (keep_going)
	{
		command = "";
		GetLine(command);
		size_t found_exit = command.find("EXIT");
		if (std::string::npos != found_exit)
		{
			if (0 == found_exit)
			{
				keep_going = false;
				continue;
			}
		}

		std::vector<std::string> results;
		QueryParse::Query(current_database, command, results);
		printf("In Database %s\n", current_database.c_str());
		for (auto& a : results)
			printf("%s\n", a.c_str());
	}

	server.StopServerThread();
	return 0;
}
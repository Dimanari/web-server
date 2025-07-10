#pragma once
#include "table.hpp"

namespace dimanari
{
	struct db_val
	{
		std::string name;
		Table table;
	};
	class Database
	{
		std::vector<db_val> tables;
	public:
		int ReadTables(const char* path);
		int WriteTables(const char* path);

		int AddTable(std::vector<std::string> keys, std::string table_name);
		int RemoveTable(std::string table_name);
		Table* GetTable(std::string table_name);
	};
};
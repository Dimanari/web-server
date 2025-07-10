#pragma once
#include <string>
#include <vector>

namespace dimanari
{
	std::vector<int> MergeVectors(const std::vector<int>& indecies1, const  std::vector<int>& indecies2, bool is_in_both);
	std::vector<int> ExcludeVector(const std::vector<int>& indecies1, const  std::vector<int>& indecies2);
	std::vector<int> Invert(const std::vector<int>& indecies1, int num_indecies);

	class QueryParse
	{
	public:

		struct SimpleCondition
		{
			int cond_type;
			int field;
			std::string value;
		};

		static SimpleCondition GetConditionFrom(std::string command, const std::vector<std::string>& keys);
		static int ConditionParse(const std::string& database, std::vector<int>& indecies, int start_index, const std::vector<std::string>& command_stack, const std::vector<std::string>& keys);

		static void TableCreate(const std::string& database, const std::vector<std::string>& keys);
		static void TableDelete(const std::string& database);
		static void TableClear(const std::string& database);
		static void TableAddValue(const std::string& database, const std::vector<std::string>& values);
		static void TableRemoveIndecies(const std::string& database, const std::vector<int>& indecies);
		static void TableGetIndex(const std::string& database, std::vector<std::string>& values, int index);

		static void TableCondition(const std::string& database, std::vector<int>& indecies, SimpleCondition cond);
		static void TableIndecies(const std::string& database, std::vector<int>& indecies);
		
		static void FormatPrint(const std::vector<std::string>& values, const std::vector<std::string>& keys, const std::string& format, std::string& output);
		static void TableFormatTable(const std::string& database, const std::string& format, std::vector<std::string>& values);
		static void TableFormatCondition(const std::string& database, const std::string& format, std::vector<std::string>& values, SimpleCondition cond);
		static void TableKeys(const std::string& database, std::vector<std::string>& keys);
		static int  TableNumEntries(const std::string& database);

		static void LoadDB(const char* path);
		static void SaveDB(const char* path);


		static void CommandDB(const std::string& database, std::vector<std::string>& responses, const std::vector<std::string>& LineTokens);

		static 	void TokenizeLine(const char* line, std::vector<std::string>& LineTokens);

		static void Query(std::string& database, const std::string command, std::vector<std::string>& __out strings);
	};
}
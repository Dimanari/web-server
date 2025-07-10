#include <utility/singleton.hpp>
#include <database/database.hpp>
#include "query.hpp"

namespace dimanari
{
	std::vector<std::string> ParseKeys(const char* command);
	std::vector<std::string> ParseSingleKey(const char* command);


	void QueryParse::LoadDB(const char* path)
	{
		Database* db = Singleton<Database>::GetInstance();

		int tables = db->ReadTables(path);
		printf("Loaded %d tables from %s\n", tables, path);
	}

	void QueryParse::SaveDB(const char* path)
	{
		Database* db = Singleton<Database>::GetInstance();

		int tables = db->WriteTables(path);
		//printf("Saves %d tables to %s\n", tables, path);
	}

	void QueryParse::CommandDB(const std::string& database, std::vector<std::string>& responses, const std::vector<std::string>& LineTokens)
	{
		// commands:
		// CREATE "key1" "key2" "key3" "key4"....
		// DELETE
		// 
		// CLEAR
		// ADD "val1" "val2" "val3" "val4"....
		// REMOVE index
		// 
		// REMOVE (indecies)
		// 
		// SELECT format condition
		// the format "key text key text" is contained inside quotations with C-style escape characters 
		// will output:
		// value text value text
		// 
		// "key=value"
		// NOT (cond)
		// (cond1) AND (cond2)
		// (cond1) OR (cond2)

		responses.clear();

		enum commands_e
		{
			CMD_CREATE_e, CMD_DELETE_e, CMD_CLEAR_e, CMD_ADD_e, CMD_REMOVE_e, CMD_SELECT_e, CMD_FORMAT_e
		};
		constexpr const char* commands_strings[] =
		{
			"CREATE", "DELETE", "CLEAR", "ADD", "REMOVE", "SELECT", "FORMAT"
		};
		constexpr int commands_amount = sizeof(commands_strings) / sizeof(char*);
		//MAKE_ENUM_AND_STRINGS_2(commands, CMD_, CREATE, DELETE, CLEAR, ADD, REMOVE, SELECT, FORMAT);

		int i = 0;
		for (; i < commands_amount; ++i)
		{
			if (0 == strncmp(commands_strings[i], LineTokens[0].c_str(), strlen(commands_strings[i])))
				break;
		}
		std::vector<std::string> params;
		for (int i = 1; i < LineTokens.size(); ++i)
			params.push_back(LineTokens[i]);

		switch (i)
		{
		case CMD_CREATE_e:
		{
			TableCreate(database, params);
			responses.push_back("Created Table " + database + " with " + std::to_string(params.size()) + " keys");
		}
			break;
		case CMD_DELETE_e:
			TableDelete(database);
			responses.push_back("Deleted Table " + database);
			break;
		case CMD_CLEAR_e:
			TableClear(database);
			responses.push_back("Cleared Table " + database);
			break;
		case CMD_ADD_e:
		{
			TableAddValue(database, params);
			responses.push_back("Added Value " + params[0] + " to " + database);
		}
			break;
		case CMD_REMOVE_e:
		{
			std::vector<std::string> table_keys;
			TableKeys(database, table_keys);

			std::vector<std::string> params_cond;
			for (int i = 1; i < params.size(); ++i)
			{
				if ("=" == params[i])
				{
					// merge equal to condition
					params_cond.back() = params_cond.back() + "=" + params[i + 1];
					i++;
				}
				else
					params_cond.push_back(params[i]);
			}

			std::vector<int> indecies_to_remove;

			//assuming simple condition
			ConditionParse(database, indecies_to_remove, 0, params_cond, table_keys);
			// remove indecies
			TableRemoveIndecies(database, indecies_to_remove);
		}
			break;
		case CMD_SELECT_e:
		{
			std::vector<std::string> table_keys;
			TableKeys(database, table_keys);

			std::vector<std::string> params_cond;
			for (int i = 1; i < params.size(); ++i)
			{
				if ("=" == params[i])
				{
					// merge equal to condition
					params_cond.back() = params_cond.back() + "=" + params[i + 1];
					i++;
				}
				else
					params_cond.push_back(params[i]);
			}

			std::vector<int> indecies_to_print;

			//assuming simple condition
			ConditionParse(database, indecies_to_print, 0, params_cond, table_keys);

			for (auto& a : indecies_to_print)
			{
				std::string response = "";
				std::vector<std::string> values;
				TableGetIndex(database, values, a);
				FormatPrint(values, table_keys, params[0], response);
				responses.push_back(response);
			}
		}
			break;
		case CMD_FORMAT_e:
		{
			TableFormatTable(database, params[0], responses);
		}
			break;
		}
	}

	void QueryParse::TokenizeLine(const char* line, std::vector<std::string>& LineTokens)
	{
		LineTokens = ParseKeys(line);
	}

	void QueryParse::Query(std::string& database, const std::string command, std::vector<std::string>& __out strings)
	{
		std::vector<std::string> strings_actual;

		QueryParse::TokenizeLine(command.c_str(), strings);

		strings_actual.clear();
		for (int i = 0; i < strings.size(); ++i)
		{
			auto& a = strings[i];
			if ("FROM" == a && (i + 1) < strings.size())
			{
				database = strings[i + 1];
				i += 1;
				continue;
			}
			strings_actual.push_back(a);
		}
		strings.clear();

		QueryParse::CommandDB(database, strings, strings_actual);
	}

	extern char EscapeChar(char src);
	bool IsStringValueQuery(char character);

	bool IsOperand(char operand)
	{
		if ('(' == operand || ')' == operand || '=' == operand) return true;
		return false;
	}

	std::vector<std::string> ParseKeys(const char* command)
	{
		std::vector<std::string> vec;
		std::string immediate = "";
		
		const char* line = command;


		bool is_string_literal = false;
		bool is_escape_char = false;
		
		vec.clear();

		while (*line)
		{
			if (is_string_literal)
			{
				if (is_escape_char)
				{
					immediate += EscapeChar(*line);
					is_escape_char = false;
				}
				else  if ('\\' == *line)
				{
					is_escape_char = true;
				}
				else if ('"' == *line)
				{
					vec.push_back(immediate);
					is_string_literal = false;
					immediate = "";
				}
				else
				{
					immediate += *line;
				}
			}
			else
			{
				if ('"' == *line)
				{
					if ("" != immediate)
					{
						vec.push_back(immediate);
						immediate = "";
					}
					is_string_literal = true;
				}
				else if (IsStringValueQuery(*line))
				{
					immediate += *line;
				}
				else
				{
					if ("" != immediate)
					{
						vec.push_back(immediate);
						immediate = "";
					}
					if (IsOperand(*line))
					{
						immediate += *line;
						vec.push_back(immediate);
						immediate = "";
					}
				}
			}
			++line;
		}

		if ("" != immediate)
		{
			vec.push_back(immediate);
			immediate = "";
		}
		return vec;
	}

	std::vector<std::string> ParseSingleKey(const char* command)
	{
		std::vector<std::string> vec;
		std::string immediate = "";

		const char* line = command;


		bool is_string_literal = false;
		bool is_escape_char = false;

		vec.clear();

		while (*line && 0 == vec.size())
		{
			if (is_string_literal)
			{
				if (is_escape_char)
				{
					immediate += EscapeChar(*line);
					is_escape_char = false;
				}
				else  if ('\\' == *line)
				{
					is_escape_char = true;
				}
				else if ('"' == *line)
				{
					vec.push_back(immediate);
					is_string_literal = false;
					immediate = "";
				}
				else
				{
					immediate += *line;
				}
			}
			else
			{
				if ('"' == *line)
				{
					if ("" != immediate)
					{
						vec.push_back(immediate);
						immediate = "";
					}
					is_escape_char = true;
				}
				else if (IsStringValueQuery(*line))
				{
					immediate += *line;
				}
				else
				{
					if ("" != immediate)
					{
						vec.push_back(immediate);
						immediate = "";
					}
				}
			}
			++line;
		}

		vec.push_back(line);

		return vec;
	}

	QueryParse::SimpleCondition QueryParse::GetConditionFrom(std::string str, const std::vector<std::string>& keys)
	{
		SimpleCondition cond;
		std::string _field = "";
		bool is_value = false;
		
		cond.field = -1;

		for (int i = 0; i < str.size(); ++i)
		{
			if (!is_value && '=' == str[i])
				is_value = true;
			else
			if(!is_value)
				_field += str[i];
			else
				cond.value += str[i];
		}
		
		for (int i = 0; i < keys.size(); ++i)
		{
			if (_field == keys[i])
			{
				cond.field = i;
				break;
			}
		}

		return cond;
	}

	int QueryParse::ConditionParse(const std::string& database, std::vector<int>& indecies, int start_index, const std::vector<std::string>& command_stack, const std::vector<std::string>& keys)
	{
		constexpr const char* keywords[] =
		{
			"AND", "OR", "NOT"
		};
		constexpr int num_keywords = sizeof(keywords) / sizeof(char*);

		enum index_operation
		{
			IND_SET, IND_AND, IND_OR
		};
		bool is_not = false;
		bool perform_operation = false;
		bool found_keyword = false;
		index_operation current_operation = IND_SET;

		for (int i = start_index; i < command_stack.size(); ++i)
		{
			std::vector<int> local_indecies;
			if (")" == command_stack[i])
			{
				return i;
			}
			if ("(" == command_stack[i])
			{
				local_indecies.clear();
				i = ConditionParse(database, local_indecies, i + 1, command_stack, keys);
				perform_operation = true;
			}
			for(int k=0;k<num_keywords;++k)
				if (keywords[k] == command_stack[i])
				{
					found_keyword = true;
					switch (k)
					{
					case 0:
						current_operation = IND_AND;
						break;
					case 1:
						current_operation = IND_OR;
						break;
					case 2:
						is_not = !is_not;
						break;
					}
					break;
				}
			if (!found_keyword && !perform_operation)
			{
				TableCondition(database, local_indecies, GetConditionFrom(command_stack[i], keys));

				if (is_not)
				{
					is_not = false;
					local_indecies = Invert(local_indecies, TableNumEntries(database));
				}

				perform_operation = true;
			}
			if (perform_operation)
			{
				switch (current_operation)
				{
				case IND_SET:
					indecies = local_indecies;
					break;
				case IND_AND:
					indecies = MergeVectors(indecies, local_indecies, true);
					break;
				case IND_OR:
					indecies = MergeVectors(indecies, local_indecies, false);
					break;
				}

				current_operation = IND_SET;
			}
			perform_operation = false;
			found_keyword = false;
		}

		return 0;
	}
	
	/*
	{
		constexpr const char* keywords[] =
		{
			"AND", "OR", "NOT"
		};
		constexpr int num_keywords = sizeof(keywords) / sizeof(char*);
		int local_index = index;

		return local_index;
	}
	*/

	void QueryParse::TableCreate(const std::string& database, const std::vector<std::string>& keys)
	{
		Database* db = Singleton<Database>::GetInstance();
		db->AddTable(keys, database);
	}

	void QueryParse::TableDelete(const std::string& database)
	{
		Database* db = Singleton<Database>::GetInstance();
		db->RemoveTable(database);
	}

	void QueryParse::TableClear(const std::string& database)
	{
		Database* db = Singleton<Database>::GetInstance();
		Table* table = db->GetTable(database);
		if (nullptr == table)
			return;
		table->ClearData();
	}

	void QueryParse::TableAddValue(const std::string& database, const std::vector<std::string>& values)
	{
		Database* db = Singleton<Database>::GetInstance();
		Table* table = db->GetTable(database);

		if (nullptr == table)
			return;
		table->AddEntry(values);
	}

	void QueryParse::TableRemoveIndecies(const std::string& database, const std::vector<int>& indecies)
	{
		Database* db = Singleton<Database>::GetInstance();
		Table* table = db->GetTable(database);

		if (nullptr == table)
			return;
		table->Remove(indecies);
	}

	void QueryParse::TableGetIndex(const std::string& database, std::vector<std::string>& values, int index)
	{
		Database* db = Singleton<Database>::GetInstance();
		Table* table = db->GetTable(database);

		if (nullptr == table)
			return;
		table->GetEntry(index, values);
	}

	void QueryParse::TableFormatTable(const std::string& database, const std::string& format, std::vector<std::string>& values)
	{
		Database* db = Singleton<Database>::GetInstance();
		Table* table = db->GetTable(database);

		if (nullptr == table)
			return;
		int entries = table->GetNumEntries();
		std::vector<std::string> keys = table->GetKeys();
		values.clear();
		for (int index = 0; index < entries; ++index)
		{
			std::vector<std::string> data;
			std::string str_line;
			table->GetEntry(index, data);

			FormatPrint(data, keys, format, str_line);

			values.push_back(str_line);
		}
	}

	void QueryParse::TableFormatCondition(const std::string& database, const std::string& format, std::vector<std::string>& values, SimpleCondition cond)
	{
		std::vector<int> indecies;
		TableCondition(database, indecies, cond);
		Database* db = Singleton<Database>::GetInstance();
		Table* table = db->GetTable(database);

		if (nullptr == table)
			return;

		std::vector<std::string> keys = table->GetKeys();

		for (auto& a : indecies)
		{
			std::vector<std::string> data;
			std::string str_line;
			table->GetEntry(a, data);

			FormatPrint(data, keys, format, str_line);

			values.push_back(str_line);
		}
	}

	void QueryParse::TableKeys(const std::string& database, std::vector<std::string>& keys)
	{
		Database* db = Singleton<Database>::GetInstance();
		Table* table = db->GetTable(database);

		if (nullptr == table)
			return;
		keys = table->GetKeys();
	}

	int QueryParse::TableNumEntries(const std::string& database)
	{
		Database* db = Singleton<Database>::GetInstance();
		Table* table = db->GetTable(database);
		if (nullptr == table)
			return 0;
		return table->GetNumEntries();
	}

	void QueryParse::TableCondition(const std::string& database, std::vector<int>& indecies, SimpleCondition cond)
	{
		Database* db = Singleton<Database>::GetInstance();
		Table* table = db->GetTable(database);
		indecies.clear();
		if (nullptr == table)
			return;
		if(-1 != cond.field)
			table->Lookup(cond.field, cond.value, indecies);
	}

	void QueryParse::TableIndecies(const std::string& database, std::vector<int>& indecies)
	{
		Database* db = Singleton<Database>::GetInstance();
		Table* table = db->GetTable(database);
		if (nullptr == table)
			return;
		int num_entries = table->GetNumEntries();
		indecies.clear();
		for (int i = 0; i < num_entries; ++i)
			indecies.push_back(i);
	}

	void QueryParse::FormatPrint(const std::vector<std::string>& values, const std::vector<std::string>& keys, const std::string& format, std::string& output)
	{
		output = "";
		const char* format_ptr = format.c_str();
		bool is_field = false;
		std::string field_current = "";
		int size = format.size();
		while (size--)
		{
			if (is_field)
			{
				if (';' == *format_ptr)
				{
					for (int i = 0; i < keys.size(); ++i)
					{
						if (keys[i] == field_current)
						{
							output += values[i];
							break;
						}
					}
					field_current = "";
					is_field = false;
				}
				else
				{
					field_current += *format_ptr;
				}
			}
			else
			{
				if ('%' == *format_ptr)
				{
					is_field = true;
				}
				else
				{
					output += *format_ptr;
				}
			}

			format_ptr++;
		}
	}

	bool IsStringValueQuery(char character)
	{
		constexpr bool lut[256] =
		{
			//00-09
			false, false, false, false, false,
			false, false, false, false, false,

			//10-19
			false, false, false, false, false,
			false, false, false, false, false,

			//20-29
			false, false, false, false, false,
			false, false, false, false, false,

			//30-39
			false, false, false, false, false,
			false, false, false, false, false,

			//40-49
			false, false, false, false, false,
			false, false, false, true, true,

			//50-59
			true, true, true, true, true,
			true, true, true, false, false,

			//60-69
			false, false, false, false, false,
			true, true, true, true, true,

			//70-79
			true, true, true, true, true,
			true, true, true, true, true,

			//80-89
			true, true, true, true, true,
			true, true, true, true, true,

			//90-99
			true, false, false, false, false,
			true, false, true, true, true,

			//100-109
			true, true, true, true, true,
			true, true, true, true, true,

			//110-119
			true, true, true, true, true,
			true, true, true, true, true,

			//120-129
			true, true, true, false, false,
			false, false, false, false, false
		};

		return lut[character];
	}

	std::vector<int> Invert(const std::vector<int>& indecies1, int num_indecies)
	{
		std::vector<int> my_vec;
		int j = 0;
		for (int i = 0; i < num_indecies; ++i)
		{
			if (j < indecies1.size() && indecies1[j] == i)
				j++;
			else
				my_vec.push_back(i);
		}
		return my_vec;
	}
	std::vector<int> MergeVectors(const std::vector<int>& indecies1, const  std::vector<int>& indecies2, bool is_in_both)
	{
		std::vector<int> merged;
		int ind1 = 0, ind2 = 0;
		while (ind1 < indecies1.size() && ind2 < indecies2.size())
		{
			if (indecies1[ind1] == indecies2[ind2])
			{
				merged.push_back(indecies1[ind1]);
				ind1++;
				ind2++;
			}
			else if (indecies1[ind1] > indecies2[ind2])
			{
				if(!is_in_both)
					merged.push_back(indecies2[ind2]);
				ind2++;
			}
			else
			{
				if (!is_in_both)
					merged.push_back(indecies1[ind1]);
				ind1++;
			}
		}

		while (ind1 < indecies1.size())
		{
			if (!is_in_both)
				merged.push_back(indecies1[ind1]);
			ind1++;
		}

		while (ind2 < indecies2.size())
		{
			if (!is_in_both)
				merged.push_back(indecies2[ind2]);
			ind2++;
		}
		return merged;
	}
	std::vector<int> ExcludeVector(const std::vector<int>& indecies1, const std::vector<int>& indecies2)
	{
		std::vector<int> merged;
		int ind1 = 0, ind2 = 0;

		while (ind1 < indecies1.size() && ind2 < indecies2.size())
		{
			if (indecies1[ind1] == indecies2[ind2])
			{
				ind1++;
				ind2++;
			}
			else if (indecies1[ind1] > indecies2[ind2])
			{
				ind2++;
			}
			else
			{
				merged.push_back(indecies1[ind1]);
				ind1++;
			}
		}

		while (ind1 < indecies1.size())
		{
			merged.push_back(indecies1[ind1]);
			ind1++;
		}
		return merged;
	}
}
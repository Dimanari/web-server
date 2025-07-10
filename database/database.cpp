#include "database.hpp"
namespace dimanari
{
	int Database::ReadTables(const char* path)
	{
		FILE* pfile = nullptr;
		int err = fopen_s(&pfile, path, "rb");
		if (nullptr == pfile)
			return 0;
		int table_count = 0;
		fread(&table_count, sizeof(int), 1, pfile);
		for (int table = 0; table < table_count; ++table)
		{
			tables.push_back(db_val());
			ReadString(tables[table].name, pfile);
			tables[table].table.LoadFromFile(pfile);
		}
		fclose(pfile);
		return tables.size();
	}

	int Database::WriteTables(const char* path)
	{
		FILE* pfile = nullptr;
		int err = fopen_s(&pfile, path, "wb");
		if (nullptr == pfile)
			return 0;
		int table_count = tables.size();
		fwrite(&table_count, sizeof(int), 1, pfile);
		for (int table = 0; table < table_count; ++table)
		{
			WriteString(tables[table].name, pfile);
			tables[table].table.SaveToFile(pfile);
		}
		fclose(pfile);
		return tables.size();
	}

	int Database::AddTable(std::vector<std::string> keys, std::string table_name)
	{
		int i = 0;
		for (; i < tables.size(); ++i)
		{
			if (tables[i].name == table_name)
				break;
		}
		if (i == tables.size())
			tables.push_back({ table_name , Table() });

		tables[i].table.InitTable(keys);

		return i;
	}

	int Database::RemoveTable(std::string table_name)
	{
		int i = 0;
		for (; i < tables.size(); ++i)
		{
			if (tables[i].name == table_name)
				break;
		}
		if (i == tables.size())
			return -1;
		std::vector<db_val> temp;
		for (int j=0; j < tables.size(); ++j)
		{
			if (i == j)
				continue;
			temp.push_back(tables[j]);
		}
		tables = temp;
		return i;
	}

	Table* Database::GetTable(std::string table_name)
	{
		int i = 0;
		for (; i < tables.size(); ++i)
		{
			if (tables[i].name == table_name)
				break;
		}
		if (i == tables.size())
			return nullptr;

		return &tables[i].table;
	}

}
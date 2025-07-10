#include "table.hpp"
namespace dimanari
{
	void ReadString(std::string& str, FILE* file)
	{
		int bytes = 0;
		fread(&bytes, sizeof(int), 1, file);
		char _buffer[512];
		if (0 < bytes)
		{
			fread(_buffer, bytes, 1, file);
			_buffer[bytes] = 0;
			str = _buffer;
		}
	}

	void WriteString(const std::string& str, FILE* file)
	{
		int bytes = str.size();
		fwrite(&bytes, sizeof(int), 1, file);
		char _buffer[512];
		if (0 < bytes)
		{
			fwrite(str.c_str(), bytes, 1, file);
		}
	}

	void Table::SaveToFile(FILE* file)
	{
		int _keys = keys.size();
		fwrite(&_keys, sizeof(int), 1, file);
		for (int k = 0; k < _keys; ++k)
		{
			WriteString(keys[k], file);
		}

		int _entries = data[0].size();
		fwrite(&_entries, sizeof(int), 1, file);

		for (int k = 0; k < _keys; ++k)
		{
			for (int e = 0; e < _entries; ++e)
			{
				WriteString(data[k][e], file);
			}
		}
	}

	void Table::LoadFromFile(FILE* file)
	{
		keys.clear();
		data.clear();
		int _keys;
		fread(&_keys, sizeof(int), 1, file);
		for (int k = 0; k < _keys; ++k)
		{
			keys.push_back("");
			ReadString(keys[k], file);
		}

		int _entries;
		fread(&_entries, sizeof(int), 1, file);

		for (int k = 0; k < _keys; ++k)
		{
			data.push_back(std::vector<std::string>());
			for (int e = 0; e < _entries; ++e)
			{
				data[k].push_back("");
				ReadString(data[k][e], file);
			}
		}
	}
}
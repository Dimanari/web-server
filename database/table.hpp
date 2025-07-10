#pragma once
#include <vector>
#include <string>
namespace dimanari
{
	class Table
	{
		std::vector<std::string> keys;
		std::vector<std::vector<std::string>> data;
	public:

		void SaveToFile(FILE* file);
		void LoadFromFile(FILE* file);

		int GetNumKeys() const { return keys.size(); }
		int GetNumEntries() const { return data[0].size(); }
		const std::vector<std::string> GetKeys() const { return keys; }

		void ClearData()
		{
			for (auto& a : data)
			{
				a.clear();
			}
		}

		void InitTable(const std::vector<std::string>& _keys)
		{
			keys.clear();
			data.clear();

			for (auto& a : _keys)
			{
				keys.push_back(a);
				data.push_back(std::vector<std::string>());
			}
		}

		int AddEntry(const std::vector<std::string>& _data)
		{
			if (_data.size() < keys.size())
				return -1;

			for (int i = 0; i < data.size(); ++i)
			{
				data[i].push_back(_data[i]);
			}
			return data[0].size() - 1;
		}

		int SetEntry(int index, const std::vector<std::string>& _data)
		{
			if (_data.size() < keys.size())
				return -1;

			for (int i = 0; i < data.size(); ++i)
			{
				data[i][index] = _data[i];
			}
			return index;
		}

		int GetEntry(int index, std::vector<std::string>& _data) const
		{
			if (data[0].size() <= index)
				return -1;
			_data.clear();

			for (int _key = 0; _key < data.size(); ++_key)
			{
				_data.push_back(data[_key][index]);
			}
			return index;

		}

		int Lookup(int _key, const std::string& value) const
		{
			if (keys.size() <= _key)
				return -1;

			for (int index = 0; index < data[_key].size(); ++index)
			{
				if (data[_key][index] == value)
					return index;
			}
			return -1;
		}

		int Lookup(int _key, const std::string& value, std::vector<int>& found) const
		{
			if (keys.size() <= _key)
				return -1;
			found.clear();

			for (int index = 0; index < data[_key].size(); ++index)
			{
				if (data[_key][index] == value)
					found.push_back(index);
			}

			return found.size();
		}


		int Remove(const std::vector<int>& indecies_to_skip)
		{

			std::vector<std::vector<std::string>> temp;
			for (int index = 0; index < keys.size(); ++index)
			{
				temp.push_back(std::vector<std::string>());
			}
			int current = 0;
			for (int index = 0; index < data[0].size(); ++index)
			{
				if (index == indecies_to_skip[current])
				{
					current++;
					continue;
				}
				for (int k = 0; k < keys.size(); ++k)
				{
					temp[k].push_back(data[k][index]);
				}
			}

			data = temp;

			return data[0].size();
		}
	};

	void ReadString(std::string& str, FILE* file);
	void WriteString(const std::string& str, FILE* file);

};
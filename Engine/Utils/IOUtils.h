// IOUtils.h
#pragma once

namespace MyEngine {

#pragma region Write

	template <class T>
	inline void Write(ostream& ofile, const T& value)
	{
		ofile.write((char*)&value, sizeof(value));
	}

	inline void Write(ostream& ofile, const string& value)
	{
		int size = (int)value.size();
		ofile.write((char*)&size, sizeof(size));

		const char *str = value.c_str();
		ofile.write((char*)str, size * sizeof(char));
	}

	template <class T>
	inline void Write(ostream& ofile, const vector<T>& value)
	{
		int size = (int)value.size();
		ofile.write((char*)&size, sizeof(size));
		for (int i = 0; i < size; i++)
			ofile.write((char*)&value[i], sizeof(value[i]));
	}

#pragma endregion


#pragma region Read

	template <class T>
	inline void Read(istream& ifile, T& value)
	{
		ifile.read((char*)&value, sizeof(value));
	}

	inline void Read(istream& ifile, string& value)
	{
		int size = 0;
		ifile.read((char*)&size, sizeof(size));

		char *str = new char[(size_t)size + 1];
		ifile.read((char*)str, size * sizeof(char));
		str[size] = '\0';
		value = str;
		delete[] str;
	}

	template <class T>
	inline void Read(istream& ifile, vector<T>& value)
	{
		int size = 0;
		ifile.read((char*)&size, sizeof(size));

		value.reserve((size_t)size);
		for (int i = 0; i < size; i++)
		{
			T t;
			ifile.read((char*)&t, sizeof(t));
			value.push_back(t);
		}
	}

#pragma endregion


#pragma region Size

	template <class T>
	inline long long SizeOf(const T&)
	{
		return sizeof(T);
	}

	inline long long SizeOf(const string& value)
	{
		return sizeof(int) + value.size() * sizeof(char);
	}

	template <class T>
	inline long long SizeOf(const vector<T>& value)
	{
		return sizeof(int) + value.size() * sizeof(T);
	}

#pragma endregion

}
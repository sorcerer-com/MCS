#pragma region Write

template <class T>
inline void Write(ostream& ofile, const T& value)
{
	ofile.write((char*)&value, sizeof(value));
}

inline void Write(ostream& ofile, const string& value)
{
	Write(ofile, value.size());
	Write(ofile, value.c_str());
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
	size_t size = 0;
	Read(ifile, size);
	char* str = new char[size + 1];
	Read(ifile, str);
	str[size] = '\0';

	value = str;
	delete[] str;
}

#pragma endregion
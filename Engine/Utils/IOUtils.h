#pragma region Write

template <class T>
inline void Write(ostream& ofile, const T& value)
{
	ofile.write((char*)&value, sizeof(value));
}

inline void Write(ostream& ofile, const string& value)
{
	size_t size = value.size();
	ofile.write((char*)&size, sizeof(size));

	const char *str = value.c_str();
	ofile.write((char*)str, size * sizeof(char));
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
	ifile.read((char*)&size, sizeof(size));

	char *str = new char[size + 1];
	ifile.read((char*)str, size * sizeof(char));
	str[size] = '\0';
	value = str;
	delete[] str;
}

#pragma endregion
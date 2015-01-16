// Utils.h
#pragma once

#include <vector>
#include <string>

namespace MyEngine {

	inline vector<string> split(const string& s, char separator, bool removeEmpty)
	{
		int i = 0, j, l = (int)s.length();
		vector<string> result;
		while (i < l) {
			j = i;
			if (separator == ' ')
				while (j < l && !isspace(s[j])) ++j;
			else
				while (j < l && s[j] != separator) ++j;
			string str = s.substr(i, j - i);
			if (!removeEmpty || (j - i > 0 && ((separator == ' ' && !isspace(str[0])) || str[0] != separator || str[0] != 0)))
				result.push_back(str);
			i = j + 1;
			if (j == l - 1 && !removeEmpty) result.push_back("");
		}
		return result;
	}

	inline string dateTimeFileName()
	{
		auto now = chrono::system_clock::now();
		time_t time = chrono::system_clock::to_time_t(now);
		tm local_tm;
		localtime_s(&local_tm, &time);

		stringstream fileName;
		fileName << setfill('0');
		fileName << setw(4) << local_tm.tm_year + 1900 << "-";
		fileName << setw(2) << local_tm.tm_mon + 1 << "-";
		fileName << setw(2) << local_tm.tm_mday << "_";
		fileName << setw(2) << local_tm.tm_hour << ".";
		fileName << setw(2) << local_tm.tm_min << ".";
		fileName << setw(2) << local_tm.tm_sec;
		return fileName.str();
	}

}
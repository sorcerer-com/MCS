// IOUtils.h
#pragma once

#include <vector>
#include <string>

namespace MyEngine {

	vector<string> split(const string& s, char separator, bool removeEmpty)
	{
		int i = 0, j, l = (int)s.length();
		vector<string> result;
		while (i < l) {
			j = i;
			if (separator == ' ')
				while (j < l && !isspace(s[j])) j++;
			else
				while (j < l && s[j] != separator) j++;
			string str = s.substr(i, j - i);
			if (!removeEmpty || (j - i > 0 && ((separator == ' ' && !isspace(str[0])) || str[0] != separator || str[0] != 0)))
				result.push_back(str);
			i = j + 1;
			if (j == l - 1 && !removeEmpty) result.push_back("");
		}
		return result;
	}

}
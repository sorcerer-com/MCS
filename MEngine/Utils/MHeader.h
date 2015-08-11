// MUtil.h
#pragma once
#pragma managed

#include <string>

using namespace System;
using namespace System::IO;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;
using namespace System::Collections::Specialized;


namespace MyEngine {

#pragma region Attributes

	[AttributeUsage(AttributeTargets::Property)]
	public ref class MPropertyAttribute : Attribute
	{
	public:
        property String^ Group;
        property String^ Name;
        property String^ SortName;
        property String^ Description;
		property bool Choosable;

		MPropertyAttribute()
		{
            this->Group = String::Empty;
            this->Name = String::Empty;
            this->SortName = String::Empty;
            this->Description = String::Empty;
			this->Choosable = false;
		}
	};

#pragma endregion

	
	static inline std::string to_string(String^ str)
	{
		const char* cstr = (const char*)(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(str)).ToPointer();
		std::string sstr = cstr;
		System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr((void*)cstr));
		return sstr;
	}

	static inline int power_of_two(int input)
	{
		int val = 1;

		while (val < input)
			val <<= 1;
		return val;
	}

}
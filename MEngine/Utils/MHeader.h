// MUtil.h
#pragma once
#pragma managed

#include <string>

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;
using namespace System::Collections::Specialized;


namespace MyEngine {
	
	static inline std::string to_string(String^ str)
	{
		const char* cstr = (const char*)(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(str)).ToPointer();
		std::string sstr = cstr;
		System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr((void*)cstr));
		return sstr;
	}

}
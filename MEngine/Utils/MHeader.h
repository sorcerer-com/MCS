// MUtil.h
#pragma once
#pragma managed

using namespace Engine;
using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;
using namespace System::Collections::Specialized;


namespace MEngine {
	
	static inline string to_string(String^ str)
	{
		const char* cstr = (const char*)(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(str)).ToPointer();
		string sstr = cstr;
		System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr((void*)cstr));
		return sstr;
	}

}
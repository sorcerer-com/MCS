// MUtil.h
#pragma once
#pragma managed

using namespace Engine;
using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections::Specialized;


namespace MEngine {

	public ref class NotifyPropertyChanged : INotifyPropertyChanged
	{
	public:
		virtual event PropertyChangedEventHandler^ PropertyChanged;

		void OnPropertyChanged(String^ info)
		{
			PropertyChanged(this, gcnew PropertyChangedEventArgs(info));
		}
	};
	
	static string to_string(String^ str)
	{
		const char* cstr = (const char*)(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(str)).ToPointer();
		string sstr = cstr;
		System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr((void*)cstr));
		return sstr;
	}

}
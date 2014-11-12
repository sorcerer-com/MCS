// ContentManager.h
#pragma once

#include <thread>

#include "..\Utils\Header.h"


namespace Engine {

	class Scene;
	class ContentElement;

	class ContentManager
	{
	public:
		using PathMapType = map < string, vector<uint> >; // path / vector with ids
		using ContentMapType = map < uint, ContentElement* > ; // id / content element

	private:
		PathMapType paths;
		ContentMapType content;

	public:
		ContentManager();
		~ContentManager();

	private:
		void doSerilization();

	};

}


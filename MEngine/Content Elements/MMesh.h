// MMesh.h
#pragma once

#include "Engine\Content Elements\Mesh.h"
#include "Engine\Managers\ContentManager.h"
#pragma managed

#include "..\Utils\MHeader.h"


namespace MEngine {

	public ref class MMesh : MContentElement
	{
	private:
		property Mesh* mesh
		{
			Mesh* get() { return (Mesh*)this->element; }
		}

	public:

	public:
		MMesh(ContentManager* owner, uint id) : 
			MContentElement(owner, id)
		{
		}
	};

}
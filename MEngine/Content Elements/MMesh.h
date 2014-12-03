// MMesh.h
#pragma once

#include "Engine\Content Elements\Mesh.h"
#pragma managed

#include "MContentElement.h"
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


		bool LoadFromOBJFile(String^ filePath)
		{
			return this->mesh->LoadFromOBJFile(to_string(filePath));
		}

		bool SaveToOBJFile(String^ filePath)
		{
			return this->mesh->SaveToOBJFile(to_string(filePath));
		}
	};

}
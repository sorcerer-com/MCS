// Mesh.cpp

#include "stdafx.h"
#include "Mesh.h"

#include "..\Utils\Config.h"


namespace Engine {

	Mesh::Mesh(ContentManager* owner, const string& name, const string& package, const string& path) : 
		ContentElement(owner, EMesh, name, package, path)
	{
	}

	Mesh::Mesh(ContentManager* owner, istream& file) : 
		ContentElement(owner, file)
	{
	}


	long long Mesh::Size() const
	{
		return 0;
	}

	void Mesh::WriteToFile(ostream& file) const
	{
	}

	ContentElement* Mesh::Clone() const
	{
		Mesh* newElem = new Mesh(*this);
		newElem->ID = INVALID_ID;
		newElem->PackageOffset = 0;
		return newElem;
	}

}
// Mesh.h
#pragma once

#include "..\Utils\Header.h"

#include "ContentElement.h"


namespace Engine {

	class Mesh : public ContentElement
	{
	public:
		Mesh(ContentManager* owner, const string& name, const string& package, const string& path);
		Mesh(ContentManager* owner, istream& file);

		virtual long long Size() const override;
		virtual void WriteToFile(ostream& file) const override;
		virtual ContentElement* Clone() const override;
	};

}


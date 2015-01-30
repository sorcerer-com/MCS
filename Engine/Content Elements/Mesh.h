// Mesh.h
#pragma once

#include "ContentElement.h"
#include "..\Utils\Header.h"
#include "..\Utils\Types\Vector3.h"


namespace MyEngine {

	struct Triangle
	{
		int vertices[3];
		int normals[3];
		int texCoords[3];
	};

	class Mesh : public ContentElement
	{
	public:
		vector<Vector3>  Vertices;
		vector<Vector3>  Normals;
		vector<Vector3>	 TexCoords;
		vector<Triangle> Triangles;

	public:
		Mesh(ContentManager* owner, const string& name, const string& package, const string& path);
		Mesh(ContentManager* owner, istream& file);

		bool LoadFromOBJFile(const string& filePath);
		bool SaveToOBJFile(const string& filePath) const;

		virtual long long Size() const override;
		virtual void WriteToFile(ostream& file) override;
		virtual ContentElement* Clone() const override;

	private:
		void init();
	};

}


// MMesh.h
#pragma once

#include "Engine\Content Elements\Mesh.h"
#pragma managed

#include "MContentElement.h"
#include "..\Utils\MHeader.h"
#include "..\Utils\Types\MPoint.h"


namespace MEngine {

	public value struct MTriangle
	{
		property array<int>^ Vertices;
		property array<int>^ Normals;
		property array<int>^ TexCoords;
	};

	public ref class MMesh : MContentElement
	{
	private:
		property Mesh* mesh
		{
			Mesh* get() { return (Mesh*)this->element; }
		}

	public:
		property List<MPoint>^ Vertices
		{
			List<MPoint>^ get()
			{
				List<MPoint>^ collection = gcnew List<MPoint>();

				auto vertices = mesh->Vertices;
				for (auto& vertex : vertices)
					collection->Add(MPoint(vertex));
				return collection;
			}
		}

		property List<MPoint>^ Normals
		{
			List<MPoint>^ get()
			{
				List<MPoint>^ collection = gcnew List<MPoint>();

				auto normals = mesh->Normals;
				for (auto& normal : normals)
					collection->Add(MPoint(normal));
				return collection;
			}
		}

		property List<MPoint>^ TexCoords
		{
			List<MPoint>^ get()
			{
				List<MPoint>^ collection = gcnew List<MPoint>();

				auto texCoords = mesh->TexCoords;
				for (auto& texCoord : texCoords)
					collection->Add(MPoint(texCoord));
				return collection;
			}
		}

		property List<MTriangle>^ Triangles
		{
			List<MTriangle>^ get()
			{
				List<MTriangle>^ collection = gcnew List<MTriangle>();

				auto triangles = mesh->Triangles;
				for (auto& triangle : triangles)
				{
					MTriangle mt;
					mt.Vertices = gcnew array<int>(3);
					mt.Normals = gcnew array<int>(3);
					mt.TexCoords = gcnew array<int>(3);
					for (int j = 0; j < 3; j++)
					{
						mt.Vertices[j] = triangle.vertices[j];
						mt.Normals[j] = triangle.normals[j];
						mt.TexCoords[j] = triangle.texCoords[j];
					}
					collection->Add(mt);
				}
				return collection;
			}
		}

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
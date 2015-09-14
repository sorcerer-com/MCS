// MMesh.h
#pragma once

#include "Engine\Content Elements\Mesh.h"
#pragma managed

#include "MContentElement.h"
#include "..\Utils\MHeader.h"
#include "..\Utils\Types\MPoint.h"


namespace MyEngine {

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
            Mesh* get() { return (Mesh*)this->contentElement; }
		}

    public:
#pragma region Mesh Properties
        [MPropertyAttribute(Group = "Shape")]
        property List<MPoint>^ Vertices
        {
            List<MPoint>^ get()
            {
                List<MPoint>^ collection = gcnew List<MPoint>();
                const auto& res = this->mesh->Vertices;
                for (const auto& value : res)
                    collection->Add(MPoint(value));
                return collection;
            }
        }
        
        [MPropertyAttribute(Group = "Shape")]
        property List<MPoint>^ Normals
        {
            List<MPoint>^ get()
            {
                List<MPoint>^ collection = gcnew List<MPoint>();
                const auto& res = this->mesh->Normals;
                for (const auto& value : res)
                    collection->Add(MPoint(value));
                return collection;
            }
        }
        
        [MPropertyAttribute(Group = "Shape")]
        property List<MPoint>^ TexCoords
        {
            List<MPoint>^ get()
            {
                List<MPoint>^ collection = gcnew List<MPoint>();
                const auto& res = this->mesh->TexCoords;
                for (const auto& value : res)
                    collection->Add(MPoint(value));
                return collection;
            }
        }
#pragma endregion

		[MPropertyAttribute(Group = "Shape")]
		property List<MTriangle>^ Triangles
		{
			List<MTriangle>^ get()
			{
				List<MTriangle>^ collection = gcnew List<MTriangle>();
                for (const auto& triangle : this->mesh->Triangles)
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


#pragma region Mesh Functions
#pragma endregion


		bool LoadFromFile(String^ filePath) override
		{
            return this->mesh->LoadFromOBJFile(to_string(filePath));
		}

		bool SaveToFile(String^ filePath) override
		{
            return this->mesh->SaveToOBJFile(to_string(filePath));
		}
	
	};

}

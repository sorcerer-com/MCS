// Mesh.cpp

#include "stdafx.h"
#include "Mesh.h"

#include "..\Scene.h"
#include "..\Utils\Config.h"
#include "..\Utils\Utils.h"
#include "..\Utils\IOUtils.h"


namespace Engine {

	Mesh::Mesh(ContentManager* owner, const string& name, const string& package, const string& path) :
		ContentElement(owner, EMesh, name, package, path)
	{
		this->IsLoaded = true;
	}

	Mesh::Mesh(ContentManager* owner, istream& file) :
		ContentElement(owner, file)
	{
		if (this->Version >= 1)
		{
			Read(file, this->Vertices);
			Read(file, this->Normals);
			Read(file, this->TexCoords);
			Read(file, this->Triangles);
		}
		this->IsLoaded = true;
	}


	bool Mesh::LoadFromOBJFile(const string& filePath)
	{
		ifstream ifile(filePath);
		if (!ifile || !ifile.is_open())
		{
			Scene::Log(EError, "Mesh", "Cannot load obj file: " + filePath);
			ifile.close();
			return false;
		}

		this->Vertices.clear();
		this->Normals.clear();
		this->TexCoords.clear();
		this->Triangles.clear();

		this->Vertices.push_back(Vector3());
		this->Normals.push_back(Vector3());
		this->TexCoords.push_back(Vector3());

		string line = "";
		vector<string> tokens;
		while (!ifile.eof())
		{
			line = "";
			tokens.clear();

			getline(ifile, line);
			if (line.size() == 0 || line[0] == '#') continue; // comment or empty line

			tokens = split(line, ' ', true);
			if (tokens.size() < 3) continue;

			if (tokens[0] == "v")
			{
				this->Vertices.push_back(Vector3(
					stof(tokens[1]),
					stof(tokens[2]),
					stof(tokens[3])));
			}
			else if (tokens[0] == "vn")
			{
				Vector3 temp(
					stof(tokens[1]), 
					stof(tokens[2]), 
					stof(tokens[3]));
				temp.normalize();
				this->Normals.push_back(temp);
			}
			else if (tokens[0] == "vt")
			{
				this->TexCoords.push_back(Vector3(
					stof(tokens[1]), 
					stof(tokens[2]), 
					0.0f));
			}
			else if (tokens[0] == "f")
			{
				for (int j = 2; j <= (int)tokens.size() - 2; j++)
				{
					string temp[3] = { tokens[1], tokens[j], tokens[j + 1] };
					Triangle t;
					for (int i = 0; i < 3; i++)
					{
						vector<string> things = split(temp[i], '/', true);
						t.vertices[i] = stoi(things[0]);
						t.texCoords[i] = 0;
						t.normals[i] = 0;
						if (things.size() > 1) t.texCoords[i] = stoi(things[1]);
						if (things.size() > 2) t.normals[i] = stoi(things[2]);
					}
					this->Triangles.push_back(t);
				}
			}
		}

		ifile.close();

		Scene::Log(ELog, "Mesh", "Load obj file: " + filePath);
		return true;
	}

	bool Mesh::SaveToOBJFile(const string& filePath) const
	{
		ofstream ofile(filePath);
		if (!ofile || !ofile.is_open())
		{
			Scene::Log(EError, "Mesh", "Cannot save obj file: " + filePath);
			ofile.close();
			return false;
		}
		ofile.precision(6);
		ofile.setf(std::ios::fixed, std::ios::floatfield);

		ofile << "# My Creative Studio" << endl;
		ofile << endl;

		int size = (int)this->Vertices.size();
		ofile << "# No. points " << (size - 1) << ":" << endl;
		for (int i = 1; i < size; i++)
			ofile << "v " << this->Vertices[i].x << " " << this->Vertices[i].y << " " << this->Vertices[i].z << endl;
		ofile << endl;

		size = (int)this->Normals.size();
		ofile << "# No. normals " << (size - 1) << ":" << endl;
		for (int i = 1; i < size; i++)
			ofile << "vn " << this->Normals[i].x << " " << this->Normals[i].y << " " << this->Normals[i].z << endl;
		ofile << endl;

		size = (int)this->TexCoords.size();
		ofile << "# No. texture coordinates " << (size - 1) << ":" << endl;
		for (int i = 1; i < size; i++)
			ofile << "vt " << this->TexCoords[i].x << " " << this->TexCoords[i].y << endl;
		ofile << endl;

		size = (int)this->Triangles.size();
		ofile << "# No. faces " << size << ":" << endl;
		for (int i = 0; i < size; i++)
		{
			ofile << "f";
			for (int j = 0; j < 3; j++)
			{
				ofile << " " <<
					this->Triangles[i].vertices[j] << "/" <<
					this->Triangles[i].texCoords[j] << "/" <<
					this->Triangles[i].normals[j];
			}
			ofile << endl;
		}

		Scene::Log(ELog, "Mesh", "Save obj file: " + filePath);
		ofile.close();

		return true;
	}


	long long Mesh::Size() const
	{
		long long size = ContentElement::Size();
		size += SizeOf(this->Vertices);
		size += SizeOf(this->Normals);
		size += SizeOf(this->TexCoords);
		size += SizeOf(this->Triangles);
		return size;
	}

	void Mesh::WriteToFile(ostream& file) const
	{
		ContentElement::WriteToFile(file);

		Write(file, this->Vertices);
		Write(file, this->Normals);
		Write(file, this->TexCoords);
		Write(file, this->Triangles);
		file.flush();
	}

	ContentElement* Mesh::Clone() const
	{
		Mesh* newElem = new Mesh(*this);
		newElem->ID = INVALID_ID;
		newElem->PackageOffset = 0;
		return newElem;
	}

}
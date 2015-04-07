// MMaterial.h
#pragma once

#include "Engine\Content Elements\Material.h"
#pragma managed

#include "MContentElement.h"
#include "..\Utils\MHeader.h"
#include "..\Utils\Types\MColor.h"

using namespace System::Xml;


namespace MyEngine {

	public ref class MMaterial : MContentElement
	{
	private:
		property Material* material
		{
			Material* get() { return (Material*)this->element; }
		}

	public:
		[MPropertyAttribute(Group = "Colors")]
		property MColor AmbientColor
		{
			MColor get() { return MColor(material->AmbientColor); }
			void set(MColor value) { material->AmbientColor = value.ToColor4(); OnChanged(); }
		}

		[MPropertyAttribute(Group = "Colors")]
		property MColor DiffuseColor
		{
			MColor get() { return MColor(material->DiffuseColor); }
			void set(MColor value) { material->DiffuseColor = value.ToColor4(); OnChanged(); }
		}

		[MPropertyAttribute(Group = "Colors")]
		property MColor SpecularColor
		{
			MColor get() { return MColor(material->SpecularColor); }
			void set(MColor value) { material->SpecularColor = value.ToColor4(); OnChanged(); }
		}

		[MPropertyAttribute(Group = "Characteristics")]
		property double Shininess
		{
			double get() { return material->Shininess; }
			void set(double value) { material->Shininess = (float)value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Characteristics")]
		property double Glossiness
		{
			double get() { return material->Glossiness; }
			void set(double value) { material->Glossiness = (float)value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Textures", Choosable = true)]
		property MContentElement^ Texture
		{
			MContentElement^ get() { return MContentManager::GetMContentElement(material->GetTexture()); }
			void set(MContentElement^ value) { if (value != nullptr) material->TextureID = value->ID; else material->TextureID = 0; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Textures", Choosable = true)]
		property MContentElement^ Bumpmap
		{
			MContentElement^ get() { return MContentManager::GetMContentElement(material->GetBumpmap()); }
			void set(MContentElement^ value) { if (value != nullptr) material->BumpmapID = value->ID; else material->BumpmapID = 0; OnChanged(); }
		}

	public:
		MMaterial(ContentManager* owner, uint id) :
			MContentElement(owner, id)
		{
		}


		bool LoadFromFile(String^ filePath) override
		{
			try
			{
				XmlDocument^ doc = gcnew XmlDocument();
				doc->Load(filePath);
				XmlElement^ docRoot = doc->DocumentElement; // Material
				if (docRoot->Name != "Material")
					return false;

				for (int i = 0; i < docRoot->ChildNodes->Count; i++)
				{
					XmlElement^ docElem = (XmlElement^)docRoot->ChildNodes[i];

					if (docElem->Name == "AmbientColor" || 
						docElem->Name == "DiffuseColor" ||
						docElem->Name == "SpecularColor")
					{
						double r = 0.0, g = 0.0, b = 0.0, a = 0.0;
						Double::TryParse(docElem->GetAttribute("R"), r);
						Double::TryParse(docElem->GetAttribute("G"), g);
						Double::TryParse(docElem->GetAttribute("B"), b);
						Double::TryParse(docElem->GetAttribute("A"), a);

						if (docElem->Name == "AmbientColor")
							this->AmbientColor = MColor(r, g, b, a);
						else if (docElem->Name == "DiffuseColor")
							this->DiffuseColor = MColor(r, g, b, a);
						else if (docElem->Name == "SpecularColor")
							this->SpecularColor = MColor(r, g, b, a);
					}
					else if (docElem->Name == "Shininess")
					{
						double value = 0.0;
						Double::TryParse(docElem->GetAttribute("Value"), value);
						this->Shininess = value;
					}
					else if (docElem->Name == "Glossiness")
					{
						double value = 0.0;
						Double::TryParse(docElem->GetAttribute("Value"), value);
						this->Glossiness = value;
					}
					else if (docElem->Name == "Texture")
					{
						int value = 0;
						Int32::TryParse(docElem->GetAttribute("Value"), value);
						this->Texture = MContentManager::GetMContentElement(this->owner->GetElement(value, true));
					}
					else if (docElem->Name == "BumpMap")
					{
						int value = 0;
						Int32::TryParse(docElem->GetAttribute("Value"), value);
						this->Bumpmap = MContentManager::GetMContentElement(this->owner->GetElement(value, true));
					}
				}
			}
			catch (...)
			{
				return false;
			}
			return true;
		}

		bool SaveToFile(String^ filePath) override
		{
			try
			{
				XmlDocument^ doc = gcnew XmlDocument();

				XmlNode^ docNode = doc->CreateXmlDeclaration("1.0", nullptr, nullptr);
				doc->AppendChild(docNode);

				XmlComment^ docComment = doc->CreateComment("My Creative Studio");
				doc->AppendChild(docComment);

				XmlElement^ docRoot = doc->CreateElement("Material");
				doc->AppendChild(docRoot);

				XmlElement^ docElem = doc->CreateElement("AmbientColor");
				docElem->SetAttribute("R", this->AmbientColor.R.ToString("0.000000"));
				docElem->SetAttribute("G", this->AmbientColor.G.ToString("0.000000"));
				docElem->SetAttribute("B", this->AmbientColor.B.ToString("0.000000"));
				docElem->SetAttribute("A", this->AmbientColor.A.ToString("0.000000"));
				docRoot->AppendChild(docElem);

				docElem = doc->CreateElement("DiffuseColor");
				docElem->SetAttribute("R", this->DiffuseColor.R.ToString("0.000000"));
				docElem->SetAttribute("G", this->DiffuseColor.G.ToString("0.000000"));
				docElem->SetAttribute("B", this->DiffuseColor.B.ToString("0.000000"));
				docElem->SetAttribute("A", this->DiffuseColor.A.ToString("0.000000"));
				docRoot->AppendChild(docElem);

				docElem = doc->CreateElement("SpecularColor");
				docElem->SetAttribute("R", this->SpecularColor.R.ToString("0.000000"));
				docElem->SetAttribute("G", this->SpecularColor.G.ToString("0.000000"));
				docElem->SetAttribute("B", this->SpecularColor.B.ToString("0.000000"));
				docElem->SetAttribute("A", this->SpecularColor.A.ToString("0.000000"));
				docRoot->AppendChild(docElem);

				docElem = doc->CreateElement("Shininess");
				docElem->SetAttribute("Value", this->Shininess.ToString("0.000000"));
				docRoot->AppendChild(docElem);

				docElem = doc->CreateElement("Glossiness");
				docElem->SetAttribute("Value", this->Glossiness.ToString("0.000000"));
				docRoot->AppendChild(docElem);

				if (this->Texture != nullptr)
				{
					docElem = doc->CreateElement("Texture");
					docElem->SetAttribute("Value", this->Texture->ID.ToString());
					docRoot->AppendChild(docElem);
				}

				if (this->Bumpmap != nullptr)
				{
					docElem = doc->CreateElement("BumpMap");
					docElem->SetAttribute("Value", this->Bumpmap->ID.ToString());
					docRoot->AppendChild(docElem);
				}

				doc->Save(filePath);
			}
			catch (...)
			{
				return false;
			}
			return true;
		}

	};

}
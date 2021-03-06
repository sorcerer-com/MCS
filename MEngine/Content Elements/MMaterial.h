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
            Material* get() { return (Material*)this->contentElement; }
		}

    public:
#pragma region Material Properties
        [MPropertyAttribute(Group = "Colors")]
        property MColor DiffuseColor
        {
            MColor get() { return MColor(this->material->DiffuseColor); }
            void set(MColor value) { this->material->DiffuseColor = value.ToColor4(); OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Colors")]
        property MColor SpecularColor
        {
            MColor get() { return MColor(this->material->SpecularColor); }
            void set(MColor value) { this->material->SpecularColor = value.ToColor4(); OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Colors")]
        property MColor InnerColor
        {
            MColor get() { return MColor(this->material->InnerColor); }
            void set(MColor value) { this->material->InnerColor = value.ToColor4(); OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Material")]
        property double Shininess
        {
            double get() { return this->material->Shininess; }
            void set(double value) { this->material->Shininess = (float)value; OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Material")]
        property double Glossiness
        {
            double get() { return this->material->Glossiness; }
            void set(double value) { this->material->Glossiness = (float)value; OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Material")]
        property double IOR
        {
            double get() { return this->material->IOR; }
            void set(double value) { this->material->IOR = (float)value; OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Material")]
        property double Absorption
        {
            double get() { return this->material->Absorption; }
            void set(double value) { this->material->Absorption = (float)value; OnChanged(); }
        }
#pragma endregion

        [MPropertyAttribute(Group = "Textures", Choosable = true)]
        property MContentElement^ DiffuseMap
        {
            MContentElement^ get() { return MContentManager::GetMContentElement(this->material->GetDiffuseMap()); }
            void set(MContentElement^ value) { this->material->Textures.DiffuseMapID = (value != nullptr ? value->ID : 0); OnChanged(); }
        }

        [MPropertyAttribute(Group = "Textures", Choosable = true)]
        property MContentElement^ NormalMap
        {
            MContentElement^ get() { return MContentManager::GetMContentElement(this->material->GetNormalMap()); }
            void set(MContentElement^ value) { this->material->Textures.NormalMapID = (value != nullptr ? value->ID : 0); OnChanged(); }
        }

	public:
		MMaterial(ContentManager* owner, uint id) :
			MContentElement(owner, id)
		{
        }


#pragma region Material Functions
#pragma endregion


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

					if (docElem->Name == "DiffuseColor" || 
						docElem->Name == "SpecularColor" ||
						docElem->Name == "InnerColor")
					{
						double r = 0.0, g = 0.0, b = 0.0, a = 0.0;
						Double::TryParse(docElem->GetAttribute("R"), r);
						Double::TryParse(docElem->GetAttribute("G"), g);
						Double::TryParse(docElem->GetAttribute("B"), b);
						Double::TryParse(docElem->GetAttribute("A"), a);

						if (docElem->Name == "DiffuseColor")
							this->DiffuseColor = MColor(r, g, b, a);
						else if (docElem->Name == "SpecularColor")
                            this->SpecularColor = MColor(r, g, b, a);
                        else if (docElem->Name == "InnerColor")
                            this->InnerColor = MColor(r, g, b, a);
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
                    else if (docElem->Name == "IOR")
                    {
                        double value = 0.0;
                        Double::TryParse(docElem->GetAttribute("Value"), value);
                        this->IOR = value;
                    }
                    else if (docElem->Name == "Absorption")
                    {
                        double value = 0.0;
                        Double::TryParse(docElem->GetAttribute("Value"), value);
                        this->Absorption = value;
                    }
					else if (docElem->Name == "DiffuseMap")
                    {
                        String^ value = docElem->GetAttribute("Value");
						this->DiffuseMap = MContentManager::GetMContentElement(this->owner->GetElement(to_string(value), true));
					}
					else if (docElem->Name == "NormalMap")
                    {
                        String^ value = docElem->GetAttribute("Value");
                        this->NormalMap = MContentManager::GetMContentElement(this->owner->GetElement(to_string(value), true));
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

				XmlElement^ docElem = doc->CreateElement("DiffuseColor");
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

                docElem = doc->CreateElement("InnerColor");
                docElem->SetAttribute("R", this->InnerColor.R.ToString("0.000000"));
                docElem->SetAttribute("G", this->InnerColor.G.ToString("0.000000"));
                docElem->SetAttribute("B", this->InnerColor.B.ToString("0.000000"));
                docElem->SetAttribute("A", this->InnerColor.A.ToString("0.000000"));
                docRoot->AppendChild(docElem);

				docElem = doc->CreateElement("Shininess");
				docElem->SetAttribute("Value", this->Shininess.ToString("0.000000"));
				docRoot->AppendChild(docElem);

				docElem = doc->CreateElement("Glossiness");
				docElem->SetAttribute("Value", this->Glossiness.ToString("0.000000"));
                docRoot->AppendChild(docElem);

                docElem = doc->CreateElement("IOR");
                docElem->SetAttribute("Value", this->IOR.ToString("0.000000"));
                docRoot->AppendChild(docElem);

                docElem = doc->CreateElement("Absorption");
                docElem->SetAttribute("Value", this->Absorption.ToString("0.000000"));
                docRoot->AppendChild(docElem);

				if (this->DiffuseMap != nullptr)
				{
					docElem = doc->CreateElement("DiffuseMap");
                    docElem->SetAttribute("Value", this->DiffuseMap->FullName);
					docRoot->AppendChild(docElem);
				}

				if (this->NormalMap != nullptr)
				{
					docElem = doc->CreateElement("NormalMap");
                    docElem->SetAttribute("Value", this->NormalMap->FullName);
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

// MTexture.h
#pragma once

#include "Engine\Content Elements\Texture.h"
#pragma managed

#include "MContentElement.h"
#include "..\Utils\MHeader.h"
#include "..\Utils\Types\MColor.h"

using namespace System::Drawing;


namespace MyEngine {

	public ref class MTexture : MContentElement
	{
	private:
		property Texture* texture
		{
			Texture* get() { return (Texture*)this->element; }
		}

		Bitmap^ imageCache;

	public:
		[MPropertyAttribute(Group = "Image")]
		property uint Width
		{
			uint get() { return texture->Width; }
		}

		[MPropertyAttribute(Group = "Image")]
		property uint Height
		{
			uint get() { return texture->Height; }
		}

		[MPropertyAttribute(Group = "Image")]
		property Bitmap^ Image
		{
			Bitmap^ get()
			{
				if (imageCache != nullptr)
					return this->imageCache;

				this->imageCache = gcnew Bitmap(texture->Width, texture->Height);

				for (int j = 0; j < this->imageCache->Height; j++)
					for (int i = 0; i < this->imageCache->Width; i++)
					{
						MColor c = this->GetColor(i, j);
						this->imageCache->SetPixel(i, j, c.ToColor());
					}
				return this->imageCache;
			}
			void set(Bitmap^ value)
			{
				if (value == nullptr)
					return;

				value = gcnew Bitmap(value);
				int sx2 = power_of_two(value->Width);
				int sy2 = power_of_two(value->Height);
				if (value->Width != sx2 || value->Height != sy2)
				{
					Bitmap^ bmp2 = gcnew Bitmap(value, sx2, sy2);
					delete value;
					value = bmp2;
				}

				this->texture->Init(value->Width, value->Height);
				for (int j = 0; j < value->Height; j++)
					for (int i = 0; i < value->Width; i++)
					{
						Color c = value->GetPixel(i, j);
						this->SetColor(i, j, MColor(c));
					}
				delete value;

				this->imageCache = nullptr;
				OnChanged();
			}
		}

	public:
		MTexture(ContentManager* owner, uint id) :
			MContentElement(owner, id)
		{
		}


		void Init(uint width, uint height)
		{
			width = power_of_two(width);
			height = power_of_two(height);
			this->texture->Init(width, height);
		}

		MColor GetColor(uint x, uint y)
		{
			return MColor(this->texture->GetColor(x, y));
		}

		void SetColor(uint x, uint y, MColor color)
		{
			this->texture->SetColor(x, y, color.ToColor4());
		}


		bool LoadFromFile(String^ filePath) override
		{
			if (filePath == nullptr || !File::Exists(filePath))
				return false;

			Bitmap^ bmp = gcnew Bitmap(filePath);
			if (bmp == nullptr)
				return false;

			this->Image = bmp;
			delete bmp;

			return true;
		}

		bool SaveToFile(String^ filePath) override
		{
			Bitmap^ bmp = this->Image;
			if (filePath == nullptr || bmp == nullptr)
				return false;

			bmp->Save(filePath);
			delete bmp;

			return true;
		}

	};

}
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
            Texture* get() { return (Texture*)this->contentElement; }
		}

		static Dictionary<uint, Bitmap^>^ imagesCache = gcnew Dictionary<uint, Bitmap^>();

    public:
#pragma region Texture Properties
        [MPropertyAttribute(Group = "Image")]
        property uint Width
        {
            uint get() { return this->texture->Width; }
        }
        
        [MPropertyAttribute(Group = "Image")]
        property uint Height
        {
            uint get() { return this->texture->Height; }
        }
#pragma endregion

		[MPropertyAttribute(Group = "Image")]
		property Bitmap^ Image
		{
			Bitmap^ get()
			{
				if (imagesCache->ContainsKey(this->id))
					return this->imagesCache[this->id];

                Bitmap^ bmp = gcnew Bitmap(this->texture->Width, this->texture->Height);

                Imaging::BitmapData^ data =
                    bmp->LockBits(System::Drawing::Rectangle(0, 0, bmp->Width, bmp->Height),
                    Imaging::ImageLockMode::WriteOnly, Imaging::PixelFormat::Format32bppArgb);
                byte* dataByte = (byte*)data->Scan0.ToPointer();
                for (uint i = 0; i < this->texture->Width * this->texture->Height; i++)
                {
                    // from RGBA to BGRA
                    dataByte[i * 4 + 2] = this->texture->Pixels[i * 4 + 0];
                    dataByte[i * 4 + 1] = this->texture->Pixels[i * 4 + 1];
                    dataByte[i * 4 + 0] = this->texture->Pixels[i * 4 + 2];
                    dataByte[i * 4 + 3] = this->texture->Pixels[i * 4 + 3];
                }
                bmp->UnlockBits(data);
				this->imagesCache[this->id] = bmp;
				return this->imagesCache[this->id];
			}
			void set(Bitmap^ value)
			{
				if (value == nullptr)
					return;

				Bitmap^ bmp = gcnew Bitmap(value);
				int sx2 = power_of_two(bmp->Width);
				int sy2 = power_of_two(bmp->Height);
				if (bmp->Width != sx2 || bmp->Height != sy2)
				{
					Bitmap^ bmp2 = gcnew Bitmap(bmp, sx2, sy2);
					delete bmp;
					bmp = bmp2;
				}

                this->texture->Init(bmp->Width, bmp->Height);
                Imaging::BitmapData^ data =
                    bmp->LockBits(System::Drawing::Rectangle(0, 0, bmp->Width, bmp->Height),
                    Imaging::ImageLockMode::ReadOnly, Imaging::PixelFormat::Format32bppArgb);
                byte* dataBytes = (byte*)data->Scan0.ToPointer();
                this->texture->SetBGRAData(dataBytes);
                bmp->UnlockBits(data);
				delete bmp;

				this->imagesCache->Remove(this->id);
				OnChanged();
			}
		}

	public:
		MTexture(ContentManager* owner, uint id) :
			MContentElement(owner, id)
		{
		}


#pragma region Texture Functions
#pragma endregion
		void Init(uint width, uint height)
		{
			width = power_of_two(width);
			height = power_of_two(height);
            this->texture->Init(width, height);
			this->imagesCache->Remove(this->id);
		}

		MColor GetColor(uint x, uint y)
		{
            return MColor(this->texture->GetColor(x, this->Height - y - 1)); // flip by y
		}

		void SetColor(uint x, uint y, MColor color)
		{
            this->texture->SetColor(x, this->Height - y - 1, color.ToColor4()); // flip by y
			this->imagesCache->Remove(this->id);
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

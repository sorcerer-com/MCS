// Tex.cpp

#include "stdafx.h"
#include "Texture.h"

#include "..\Engine.h"
#include "..\Utils\Config.h"
#include "..\Utils\Utils.h"
#include "..\Utils\IOUtils.h"
#include "..\Utils\External\lodepng.h"


namespace MyEngine {

	Texture::Texture(ContentManager* owner, const string& name, const string& package, const string& path) :
		ContentElement(owner, ETexture, name, package, path)
	{
		this->init();
		this->IsLoaded = true;
	}

	Texture::Texture(ContentManager* owner, istream& file) :
		ContentElement(owner, file)
	{
        this->init();
		if (this->Version >= 1)
        {
#pragma region Texture Read
        	Read(file, this->Width);
        	Read(file, this->Height);
#pragma endregion

			// read PNG data
            Read(file, this->rawDataSize);
            this->rawData = new byte[this->rawDataSize];
            for (int i = 0; i < this->rawDataSize; i++)
                Read(file, this->rawData[i]);
			
            lodepng_decode_memory(&this->Pixels, &this->Width, &this->Height, this->rawData, this->rawDataSize, LCT_RGBA, 8);
		}
		this->IsLoaded = true;
    }

    Texture::Texture(const Texture& texture) : 
        ContentElement(texture)
    {
        this->init();
        
        this->Width = texture.Width;
        this->Height = texture.Height;
        this->Pixels = new byte[this->Width * this->Height * 4];
        memcpy(this->Pixels, texture.Pixels, this->Width * this->Height * 4);

        this->rawDataSize = texture.rawDataSize;
        this->rawData = new byte[this->rawDataSize];
        memcpy(this->rawData, texture.rawData, this->rawDataSize);

        this->IsLoaded = true;
    }
    
    Texture::~Texture()
    {
        if (this->Pixels != NULL)
            delete[] this->Pixels;
        if (this->rawData != NULL)
            delete[] this->rawData;

        this->init();
    }

	void Texture::init()
    {
#pragma region Texture Init
    	this->Width = 0;
    	this->Height = 0;
    	this->Pixels = NULL;
    	this->Changed = true;
    	this->rawDataSize = 0;
    	this->rawData = NULL;
#pragma endregion
	}


	void Texture::Init(uint width, uint height)
    {
		if (this->Pixels != NULL)
			delete[] this->Pixels;
        this->Pixels = NULL;

		this->Width = width;
		this->Height = height;
		this->Pixels = new byte[width * height * 4];
		memset(this->Pixels, 255, width * height * 4);
		this->Changed = true;

        this->updateRawData();
	}

	Color4 Texture::GetColor(uint x, uint y) const
    {
		if (x >= this->Width || y >= this->Height)
			return Color4::Black();

		Color4 c;
		c.r = (float)this->Pixels[(y * this->Width + x) * 4 + 0] / 255.0f;
		c.g = (float)this->Pixels[(y * this->Width + x) * 4 + 1] / 255.0f;
		c.b = (float)this->Pixels[(y * this->Width + x) * 4 + 2] / 255.0f;
		c.a = (float)this->Pixels[(y * this->Width + x) * 4 + 3] / 255.0f;
		return c;
	}

	Color4 Texture::GetColor(float u, float v) const
	{
		u = (float)((u - floor(u)) * this->Width);
		v = (float)((v - floor(v)) * this->Height);
		uint x = (uint)(u);
		uint y = (uint)(v);
		Color4 c1 = this->GetColor(x, y);
		Color4 c2 = this->GetColor((x + 1) % this->Width, y);
		Color4 c3 = this->GetColor(x, (y + 1) % this->Height);
		Color4 c4 = this->GetColor((x + 1) % this->Width, (y + 1) % this->Height);
		u -= (int)u;
		v -= (int)v;
		return linearFilter(c1, c2, c3, c4, u, v);
	}

	void Texture::SetColor(uint x, uint y, const Color4& color)
    {
		if (x < this->Width && y < this->Height)
		{
			this->Pixels[(y * this->Width + x) * 4 + 0] = (byte)(color.r * 255);
			this->Pixels[(y * this->Width + x) * 4 + 1] = (byte)(color.g * 255);
			this->Pixels[(y * this->Width + x) * 4 + 2] = (byte)(color.b * 255);
			this->Pixels[(y * this->Width + x) * 4 + 3] = (byte)(color.a * 255);
            this->Changed = true;
		}
    }

    void Texture::SetBGRAData(const byte* data)
    {
        for (uint i = 0; i < this->Width * this->Height; i++)
        {
            // from BGRA to RGBA
            this->Pixels[i * 4 + 0] = data[i * 4 + 2];
            this->Pixels[i * 4 + 1] = data[i * 4 + 1];
            this->Pixels[i * 4 + 2] = data[i * 4 + 0];
            this->Pixels[i * 4 + 3] = data[i * 4 + 3];
        }

        this->updateRawData();
    }

    void Texture::updateRawData()
    {
        this->rawDataSize = 0;
        if (this->rawData != NULL)
            delete[] this->rawData;
        this->rawData = NULL;
        lodepng_encode_memory(&this->rawData, (size_t*)&this->rawDataSize, (byte*)this->Pixels, this->Width, this->Height, LCT_RGBA, 8);
    }


	long long Texture::Size() const
    {
        long long size = ContentElement::Size();
#pragma region Texture Size
        size += SizeOf(this->Width);
        size += SizeOf(this->Height);
#pragma endregion
        size += SizeOf(this->rawDataSize);
        size += this->rawDataSize;
		return size;
	}

	void Texture::WriteToFile(ostream& file)
    {
		ContentElement::WriteToFile(file);

#pragma region Texture Write
		Write(file, this->Width);
		Write(file, this->Height);
#pragma endregion

        Write(file, this->rawDataSize);
        for (long long i = 0; i < this->rawDataSize; i++)
            Write(file, this->rawData[i]);

		file.flush();
	}

	ContentElement* Texture::Clone() const
    {
		Texture* newElem = new Texture(*this);
		newElem->ID = INVALID_ID;
		newElem->PackageOffset = 0;
		newElem->SavedSize = 0;
		newElem->Changed = true;
		return newElem;
	}

}

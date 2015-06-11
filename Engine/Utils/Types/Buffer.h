// Image.h
#pragma once


namespace MyEngine {

    template<class T>
	struct Buffer
    {
        uint Width;
        uint Height;
        T* Data;


        Buffer()
        {
            this->Width = 0;
            this->Height = 0;
            this->Data = NULL;
        }

        Buffer(uint width, uint height) :
            Buffer()
        {
            this->Init(width, height);
        }

        Buffer(const Buffer& buffer) :
            Buffer()
        {
            this->Init(buffer);
        }

        ~Buffer()
        {
            this->Clear();
        }


        void Init(uint width, uint height)
        {
            if (this->Data != NULL)
                delete[] this->Data;

            this->Width = width;
            this->Height = height;
            this->Data = new T[width * height];
        }

        void Init(const Buffer& buffer)
        {
            if (this->Data != NULL)
                delete[] this->Data;

            this->Width = buffer.Width;
            this->Height = buffer.Height;
            this->Data = new T[buffer.Width * buffer.Height];
            memcpy(this->Data, buffer.Data, buffer.Width * buffer.Height * sizeof(T));
        }

        void Fill(const T& element)
        {
            for (int j = 0; j < (int)this->Height; j++)
                for (int i = 0; i < (int)this->Width; i++)
                    this->SetElement(i, j, element);
        }

        void Clear()
        {
            this->Width = 0;
            this->Height = 0;
            if (this->Data != NULL)
                delete[] this->Data;
            this->Data = NULL;
        }

        Buffer& operator=(const Buffer& buffer)
        {
            if (this == &buffer)
                return *this;

            this->Clear();
            this->Init(buffer);
            return *this;
        }


        T GetElement(uint x, uint y) const
        {
            if (this->Data == NULL || x >= this->Width || y >= this->Height)
                return T();

            return this->Data[y * this->Width + x];
        }

        void SetElement(uint x, uint y, const T& element)
        {
            if (this->Data == NULL || x >= this->Width || y >= this->Height)
                return;

            this->Data[y * this->Width + x] = element;
        }

	};

}
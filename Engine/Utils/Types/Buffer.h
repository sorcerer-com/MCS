// Image.h
#pragma once


namespace MyEngine {

    template<class T>
	struct Buffer
    {
        uint width;
        uint height;
        T* data;


        Buffer()
        {
            this->width = 0;
            this->height = 0;
            this->data = NULL;
        }

        Buffer(uint width, uint height) :
            Buffer()
        {
            this->init(width, height);
        }

        Buffer(const Buffer& buffer) :
            Buffer()
        {
            this->init(buffer);
        }

        ~Buffer()
        {
            this->clear();
        }


        void init(uint width, uint height)
        {
            if (this->data != NULL)
                delete[] this->data;

            this->width = width;
            this->height = height;
            this->data = new T[width * height];
            memset(this->data, 0, this->width * this->height * sizeof(T));
        }

        void init(const Buffer& buffer)
        {
            if (this->data != NULL)
                delete[] this->data;

            this->width = buffer.width;
            this->height = buffer.height;
            this->data = new T[buffer.width * buffer.height];
            memcpy(this->data, buffer.data, buffer.width * buffer.height * sizeof(T));
        }

        void fill(const T& element)
        {
            for (int j = 0; j < (int)this->height; j++)
                for (int i = 0; i < (int)this->width; i++)
                    this->setElement(i, j, element);
        }

        void clear()
        {
            this->width = 0;
            this->height = 0;
            if (this->data != NULL)
                delete[] this->data;
            this->data = NULL;
        }

        Buffer& operator=(const Buffer& buffer)
        {
            if (this == &buffer)
                return *this;

            this->clear();
            this->init(buffer);
            return *this;
        }


        T getElement(uint x, uint y) const
        {
            if (this->data == NULL || x >= this->width || y >= this->height)
                return T();

            return this->data[y * this->width + x];
        }

        void setElement(uint x, uint y, const T& element)
        {
            if (this->data == NULL || x >= this->width || y >= this->height)
                return;

            this->data[y * this->width + x] = element;
        }

	};

}
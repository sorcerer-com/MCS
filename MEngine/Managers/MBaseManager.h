// MBaseManager.h
#pragma once

#pragma managed

#include "..\Utils\MHeader.h"


namespace MyEngine {

    ref class MEngine;

    public ref class MBaseManager abstract
    {
    protected:
        MEngine^ owner;

    public:
        property MEngine^ Owner
        {
            MEngine^ get() { return this->owner;  }
        }

#pragma region BaseManager Properties
#pragma endregion


    public:
        MBaseManager(MEngine^ owner)
        {
            this->owner = owner;
        }


#pragma region BaseManager Functions
#pragma endregion

    };

};
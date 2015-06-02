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

    public:
        MBaseManager(MEngine^ owner)
        {
            this->owner = owner;
        }

    };

};
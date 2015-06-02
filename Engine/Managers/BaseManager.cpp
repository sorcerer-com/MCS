// BaseManager.cpp

#include "stdafx.h"
#include "BaseManager.h"

#include "..\Utils\Thread.h"


namespace MyEngine {

    BaseManager::BaseManager(Engine* owner)
    {
        if (!owner)
            throw "ArgumentNullException: owner";

        this->Owner = owner;

        this->thread = make_shared<Thread>();
    }

    BaseManager::~BaseManager()
    {
        this->thread->join();
    }

}
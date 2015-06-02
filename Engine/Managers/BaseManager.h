// BaseManager.h
#pragma once

#include "..\Utils\Header.h"


namespace MyEngine {

    class Engine;
    struct Thread;
    
    class BaseManager
    {
    public:
        Engine* Owner;

    protected:
        shared_ptr<Thread> thread;

    protected:
        BaseManager(Engine* owner);
        ~BaseManager();

    };

}


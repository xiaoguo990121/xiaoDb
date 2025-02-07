#pragma once

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    class Cleanable
    {
    public:
        Cleanable();

        Cleanable(Cleanable &) = delete;
        Cleanable &operator=(Cleanable &) = delete;

        ~Cleanable();

        Cleanable(Cleanable &&) noexcept;
        Cleanable &operator=(Cleanable &&) noexcept;

        using CleanupFunction = void (*)(void *arg1, void *arg2);

        void RegisterCleanup(CleanupFunction function, void *arg1, void *arg2);

        void DelegateCleanupsTo(Cleanable *other);

        inline void Reset()
        {
        }

    protected:
        struct Cleanup
        {
            CleanupFunction function;
            void *arg1;
            void *arg2;
            Cleanup *next;
        };
        Cleanup cleanup_;

        void RegisterCleanup(Cleanup *c);

    private:
        inline void DoCleanup()
        {
            if (cleanup_.function != nullptr)
            {
                (*cleanup_.function)(cleanup_.arg1, cleanup_.arg2);
                for (Cleanup *c = cleanup_.next; c != nullptr;)
                {
                    (*c->function)(c->arg1, c->arg2);
                    Cleanup *next = c->next;
                    delete c;
                    c = next;
                }
            }
        }
    };

    class SharedCleanablePtr
    {
    public:
        SharedCleanablePtr() {}
        SharedCleanablePtr(const SharedCleanablePtr &from);
        SharedCleanablePtr(SharedCleanablePtr &&from) noexcept;
        SharedCleanablePtr &operator=(const SharedCleanablePtr &from);
        SharedCleanablePtr &operator=(SharedCleanablePtr &&from) noexcept;

        ~SharedCleanablePtr();

        void Allocate();

        void Reset();

        Cleanable &operator*();
        Cleanable *operator->();

        Cleanable *get();

        void RegisterCopyWith(Cleanable *target);

        void MoveAsCleanupTo(Cleanable *target);

    private:
        struct Impl;
        Impl *ptr_ = nullptr;
    };

}
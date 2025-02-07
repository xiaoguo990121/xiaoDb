#include "xiaodb/cleanable.h"

#include <atomic>
#include <cassert>
#include <utility>

namespace XIAODB_NAMESPACE
{
    Cleanable::Cleanable()
    {
        cleanup_.function = nullptr;
        cleanup_.next = nullptr;
    }

    Cleanable::~Cleanable() { DoCleanup(); }

    Cleanable::Cleanable(Cleanable &&other) noexcept { *this = std::move(other); }

    Cleanable &Cleanable::operator=(Cleanable &&other) noexcept
    {
        assert(this != &other);
        cleanup_ = other.cleanup_;
        other.cleanup_.function = nullptr;
        other.cleanup_.next = nullptr;
        return *this;
    }

    void Cleanable::DelegateCleanupsTo(Cleanable *other)
    {
        assert(other != nullptr);
        if (cleanup_.function == nullptr)
        {
            return;
        }
        Cleanup *c = &cleanup_;
        other->RegisterCleanup(c->function, c->arg1, c->arg2);
        c = c->next;
        while (c != nullptr)
        {
            Cleanup *next = c->next;
            other->RegisterCleanup(c);
            c = next;
        }
        cleanup_.function = nullptr;
        cleanup_.next = nullptr;
    }

    void Cleanable::RegisterCleanup(Cleanable::Cleanup *c)
    {
        assert(c != nullptr);
        if (cleanup_.function == nullptr)
        {
            cleanup_.function == c->function;
            cleanup_.arg1 = c->arg1;
            cleanup_.arg2 = c->arg2;
            delete c;
        }
        else
        {
            c->next = cleanup_.next;
            cleanup_.next = c;
        }
    }

    void Cleanable::RegisterCleanup(CleanupFunction func, void *arg1, void *arg2)
    {
        assert(func != nullptr);
        Cleanup *c;
        if (cleanup_.function == nullptr)
        {
            c = &cleanup_;
        }
        else
        {
            c = new Cleanup;
            c->next = cleanup_.next;
            cleanup_.next = c;
        }
        c->function = func;
        c->arg1 = arg1;
        c->arg2 = arg2;
    }

    struct SharedCleanablePtr::Impl : public Cleanable
    {
        std::atomic<unsigned> ref_count{1};
        void Ref() { ref_count.fetch_add(1, std::memory_order_relaxed); }
        void Unref()
        {
            if (ref_count.fetch_sub(1, std::memory_order_relaxed) == 1)
            {
                delete this;
            }
        }
        static void UnrefWrapper(void *arg1, void *)
        {
            static_cast<SharedCleanablePtr::Impl *>(arg1)->Unref();
        }
    };

    void SharedCleanablePtr::Reset()
    {
        if (ptr_)
        {
            ptr_->Unref();
            ptr_ = nullptr;
        }
    }

    void SharedCleanablePtr::Allocate()
    {
        Reset();
        ptr_ = new Impl();
    }

    SharedCleanablePtr::SharedCleanablePtr(const SharedCleanablePtr &from)
    {
        *this = from;
    }

    SharedCleanablePtr::SharedCleanablePtr(SharedCleanablePtr &&from) noexcept
    {
        *this = std::move(from);
    }

    SharedCleanablePtr &SharedCleanablePtr::operator=(
        const SharedCleanablePtr &from)
    {
        if (this != &from)
        {
            Reset();
            ptr_ = from.ptr_;
            if (ptr_)
            {
                ptr_->Ref();
            }
        }
        return *this;
    }

    SharedCleanablePtr &SharedCleanablePtr::operator=(
        SharedCleanablePtr &&from) noexcept
    {
        assert(this != &from);
        Reset();
        ptr_ = from.ptr_;
        from.ptr_ = nullptr;
        return *this;
    }

    SharedCleanablePtr::~SharedCleanablePtr() { Reset(); }

    Cleanable &SharedCleanablePtr::operator*()
    {
        return *ptr_;
    }

    Cleanable *SharedCleanablePtr::operator->()
    {
        return ptr_;
    }

    Cleanable *SharedCleanablePtr::get()
    {
        return ptr_;
    }

    void SharedCleanablePtr::RegisterCopyWith(Cleanable *target)
    {
        if (ptr_)
        {
            ptr_->Ref();
            target->RegisterCleanup(&Impl::UnrefWrapper, ptr_, nullptr);
        }
    }

    void SharedCleanablePtr::MoveAsCleanupTo(Cleanable *target)
    {
        if (ptr_)
        {
            target->RegisterCleanup(&Impl::UnrefWrapper, ptr_, nullptr);
            ptr_ = nullptr;
        }
    }

}
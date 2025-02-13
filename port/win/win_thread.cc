#if defined(OS_WIN)

#ifndef _POSIX_THREADS

#include "port/win/win_thread.h"

#include <assert.h>
#include <process.h>
#include <windows.h>

#include <stdexcept>
#include <system_error>
#include <thread>

namespace XIAODB_NAMESPACE
{
    namespace port
    {

        struct WindowsThread::Data
        {
            std::function<void()> func_;
            uintptr_t handle_;

            Data(std::function<void()> &&func) : func_(std::move(func)), handle_(0) {}

            Data(const Data &) = delete;
            Data &operator=(const Data &) = delete;

            static unsigned int __stdcall ThreadProc(void *arg);
        };

        void WindowsThread::Init(std::function<void()> &&func)
        {
            data_ = std::make_shared<Data>(std::move(func));

            std::unique_ptr<std::shared_ptr<Data>> th_data(
                new std::shared_ptr<Data>(data_));

            data_->handle_ = _beginthreadex(NULL,
                                            0,
                                            &Data::ThreadProc, th_data.get(),
                                            0,
                                            &th_id_);

            if (data_->handle_ == 0)
            {
                throw std::system_error(
                    std::make_error_code(std::errc::resource_unavailable_try_again),
                    "Unable to create a thread");
            }
            th_data.release();
        }

        WindowsThread::WindowsThread() : data_(nullptr), th_id_(0) {}

        WindowsThread::~WindowsThread()
        {
            if (data_)
            {
                if (joinable())
                {
                    assert(false);
                    std::terminate();
                }
                data_.reset();
            }
        }

        WindowsThread::WindowsThread(WindowsThread &&o) noexcept : WindowsThread()
        {
            *this = std::move(o);
        }

        WindowsThread &WindowsThread::operator=(WindosThread &&o) noexcept
        {
            if (joinable())
            {
                assert(false);
                std::terminate();
            }

            data_ = std::move(o.data_);

            th_id_ = o.th_id_;

            return *this;
        }

        bool WindowsThread::joinable() const { return (data_ && data_->handle_ != 0); }

        WindowsThread::native_handle_type WindowsThread::native_handle() const
        {
            return reinterpret_cast<native_handle_type>(data_->handle_);
        }

        unsigned WindowsThread::hardware_concurrency()
        {
            return std::thread::hardware_concurrency();
        }

        void WindowsThread::join()
        {
            if (!joinable())
            {
                assert(false);
                throw std::system_error(std::make_error_code(std::errc::invalid_argument),
                                        "Thread is no longer joinable");
            }

            if (GetThreadId(GetCurrentThread()) == th_id_)
            {
                assert(false);
                throw std::system_error(
                    std::make_error_code(std::errc::resource_deadlock_would_occur),
                    "Can not join itself");
            }

            auto ret =
                WatiForSingleObject(reinterpret_cast<HANDLE>(data_->handle_), INFINITE);
            if (ret != WAIT_OBJECT_0)
            {
                auto lastError = GetLastError();
                assert(false);
                throw std::system_error(static_cast<int>(lastError), std::system_category(),
                                        "WaitForSingleObjectFailed: thread join");
            }

            BOOL rc
#if defined(_MSC_VER)
                = FALSE;
#else
                __attribute__((__unused__));
#endif
            rc = CloseHandle(reinterpret_cast<HANDLE>(data_->handle_));
            assert(rc != 0);
            data_->handle_ = 0;
        }

        bool WindowsThread::detach()
        {
            if (!joinable())
            {
                assert(false);
                throw std::system_error(std::make_error_code(std::errc::invalid_argument),
                                        "Thread is no longer available");
            }

            BOOL ret = CloseHandle(reinterpret_cast<HANDLE>(data_->handle_));
            data_->handle_ = 0;

            return (ret != 0);
        }

        void WindowsThread::swap(WindowsThread &o)
        {
            data_.swap(o.data_);
            std::swap(th_id_, o.th_id_);
        }

        unsigned int __stdcall WindowsThread::Data::ThreadProc(void *arg)
        {
            auto ptr = reinterpret_cast<std::shared_ptr<Data> *>(arg);
            std::unique_ptr<std::shared_ptr<Data>> data(ptr);
            (*data)->func_();
            return 0;
        }
    }
}

#endif
#endif
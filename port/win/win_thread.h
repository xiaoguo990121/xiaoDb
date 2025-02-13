#pragma once

#ifndef _POSIX_THREADS

#include <functional>
#include <memory>
#include <type_traits>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    namespace port
    {
        /**
         * @brief This class is a replacement for std::thread
         * 2 reasons we do not like std::thread:
         * -- is that it dynamically allocates its internals that are automatically
         *    freed when the thread terminates and not on the destruction of the
         *    object. This makes it difficult to control the source of memory allocation
         *  - This implements Pimpl so we can easily replace the guts of the
         *    object in our private version if necessary.
         */
        class WindowsThread
        {
            struct Data;

            std::shared_ptr<Data> data_;
            unsigned int th_id_;

            void Init(std::function<void()> &&);

        public:
            using native_handle_type = void *;

            WindowsThread();

            /**
             * @brief Construct a new Windows Thread object
             *
             * @tparam Fn
             * @tparam Args
             * @tparam std::enable_if<!std::is_same<
             * typename std::decay<Fn>::type, WindowsThread>::value>::type
             * @param fx
             * @param ax
             */
            template <class Fn, class... Args,
                      class = typename std::enable_if<!std::is_same<
                          typename std::decay<Fn>::type, WindowsThread>::value>::type>
            explicit WindowsThread(Fn &&fx, Args &&...ax) : WindowsThread()
            {
                auto binder = std::bind(std::forward<Fn>(fx), std::forward<Args>(ax)...);

                std::function<void()> target = binder;

                Init(std::move(target));
            }

            ~WindowsThread();

            WindowsThread(const WindowsThread &) = delete;

            WindowsThread &operator=(const WindowsThread &) = delete;

            WindowsThread(WindowsThread &&) noexcept;

            WindowsThread &operator=(WindowsThread &&) noexcept;

            bool joinable() const;

            unsigned int get_id() const { return th_id_; }

            native_handle_type native_handle() const;

            static unsigned hardware_concurrency();

            void join();

            bool detach();

            void swap(WindowsThread &);
        };
    }
}

namespace std
{
    inline void swap(XIAODB_NAMESPACE::port::WindowsThread &th1,
                     XIAODB_NAMESPACE::port::WindowsThread &th2)
    {
        th1.swap(th2);
    }
}

#endif
#pragma once

#include <stdint.h>

#include <chrono>
#include <memory>

#include "xiaodb/customizable.h"
#include "xiaodb/port_defs.h"
#include "xiaodb/xiaodb_namespace.h"
#include "xiaodb/status.h"

#ifdef _WIN32
#undef GetCurrentTime
#endif

namespace XIAODB_NAMESPACE
{
    struct ConfigOptions;

    class SystemClock : public Customizable
    {
    public:
        ~SystemClock() override {}

        static const char *Type() { return "SystemClock"; }
        static Status CreateFromString(const ConfigOptions &options,
                                       const std::string &value,
                                       std::shared_ptr<SystemClock> *result);

        const char *Name() const override = 0;

        static const char *kDefaultName() { return "DefaultClock"; }

        static const std::shared_ptr<SystemClock> &Default();

        virtual uint64_t NowMicros() = 0;

        virtual uint64_t NowNanos() { return NowMicros() * 1000; }

        virtual uint64_t CPUMicros() { return 0; }

        virtual uint64_t CPUNanos() { return CPUMicros() * 1000; }

        virtual void SleepForMicroseconds(int micros) = 0;

        virtual bool TimedWait(port::CondVar *cv, std::chrono::microseconds deadline);

        virtual Status GetCurrentTime(int64_t *unix_time) = 0;

        virtual std::string TimeToString(uint64_t time) = 0;
    };

    class SystemClockWrapper : public SystemClock
    {

    public:
        explicit SystemClockWrapper(const std::shared_ptr<SystemClock> &t);

        uint64_t NowMicros() override { return target_->NowMicros(); }

        uint64_t NowNanos() override { return target_->NowNanos(); }

        uint64_t CPUMicros() override { return target_->CPUMicros(); }

        uint64_t CPUNanos() override { return target_->CPUNanos(); }

        void SleepForMicroseconds(int micros) override
        {
            return target_->SleepForMicroseconds(micros);
        }

        bool TimedWait(port::CondVar *cv,
                       std::chrono::microseconds deadline) override
        {
            return target_->TimedWait(cv, deadline);
        }

        Status GetCurrentTime(int64_t *unix_time) override
        {
            return target_->GetCurrentTime(unix_time);
        }

        std::string TimeToString(uint64_t time) override
        {
            return target_->TimeToString(time);
        }

        Status PrepareOptions(const ConfigOptions &options) override;

        std::string SerializeOptions(const ConfigOptions &config_options,
                                     const std::string &header) const override;

        const Customizable *Inner() const override { return target_.get(); }

    protected:
        std::shared_ptr<SystemClock> target_;
    };
}
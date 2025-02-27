
#pragma once
#include "xiaodb/configurable.h"
#include "xiaodb/status.h"

namespace XIAODB_NAMESPACE
{
    class Customizable : public Configurable
    {
    public:
        ~Customizable() override {}

        virtual const char *Name() const = 0;

        virtual std::string GetId() const
        {
            std::string id = Name();
            return id;
        }

        virtual bool IsInstanceOf(const std::string &name) const
        {
            if (name.empty())
            {
                return false;
            }
            else if (name == Name())
            {
                return true;
            }
            else
            {
                const char *nickname = NickName();
                if (nickname != nullptr && name == nickname)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        const void *GetOptionsPtr(const std::string &name) const override
        {
            const void *ptr = Configurable::GetOptionsPtr(name);
            if (ptr != nullptr)
            {
                return ptr;
            }
            else
            {
                const auto inner = Inner();
                if (inner != nullptr)
                {
                    return inner->GetOptionsPtr(name);
                }
                else
                {
                    return nullptr;
                }
            }
        }

        template <typename T>
        const T *CheckedCast() const
        {
            if (IsInstanceOf(T::kClassName()))
            {
                return static_cast<const T *>(this);
            }
            else
            {
                const auto inner = Inner();
                if (inner != nullptr)
                {
                    return inner->CheckedCast<T>();
                }
                else
                {
                    return nullptr;
                }
            }
        }

        template <typename T>
        T *CheckedCast()
        {
            if (IsInstanceOf(T::kClassName()))
            {
                return static_cast<T *>(this);
            }
            else
            {
                auto inner = const_cast<Customizable *>(Inner());
                if (inner != nullptr)
                {
                    return inner->CheckedCast<T>();
                }
                else
                {
                    return nullptr;
                }
            }
        }

        bool AreEquivalent(const ConfigOptions &config_options,
                           const Configurable *other,
                           std::string *mismatch) const override;

        Status GetOption(const ConfigOptions &config_options, const std::string &name,
                         std::string *value) const override;

        static Status GetOptionsMap(
            const ConfigOptions &config_options, const Customizable *custom,
            const std::string &opt_value, std::string *id,
            std::unordered_map<std::string, std::string> *options);

        static Status ConfigureNewObject(
            const ConfigOptions &config_options, Customizable *object,
            const std::unordered_map<std::string, std::string> &options);

        virtual const Customizable *Inner() const { return nullptr; }

    protected:
        std::string GenerateIndividualId() const;

        virtual const char *NickName() const { return ""; }

        std::string GetOptionName(const std::string &long_name) const override;
        std::string SerializeOptions(const ConfigOptions &options,
                                     const std::string &prefix) const override;
    };
}
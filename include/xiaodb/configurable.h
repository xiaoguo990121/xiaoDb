#pragma once

#include <atomic>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "xiaodb/xiaodb_namespace.h"
#include "xiaodb/status.h"

namespace XIAODB_NAMESPACE
{
    class Logger;
    class ObjectRegistry;
    class OptionTypeInfo;
    struct ColumnFamilyOptions;
    struct ConfigOptions;
    struct DBOptions;

    class Configurable
    {
    protected:
        friend class ConfigurableHelper;
        struct RegisteredOptions
        {
            std::string name;
            intptr_t opt_offset;
            const std::unordered_map<std::string, OptionTypeInfo> *type_map;
        };

    public:
        virtual ~Configurable() {}

        template <typename T>
        const T *GetOptions() const
        {
            return GetOptions<T>(T::kName());
        }
        template <typename T>
        T *GetOptions()
        {
            return GetOptions<T>(T::kName());
        }
        template <typename T>
        const T *GetOptions(const std::string &name) const
        {
            return reinterpret_cast<const T *>(GetOptionsPtr(name));
        }
        template <typename T>
        T *GetOptions(const std::string &name)
        {
            // FIXME: Is this sometimes reading a raw pointer from a shared_ptr,
            // unsafely relying on the object layout?
            return reinterpret_cast<T *>(const_cast<void *>(GetOptionsPtr(name)));
        }

        Status ConfigureFromMap(
            const ConfigOptions &config_options,
            const std::unordered_map<std::string, std::string> &opt_map);
        Status ConfigureFromMap(
            const ConfigOptions &config_options,
            const std::unordered_map<std::string, std::string> &opt_map,
            std::unordered_map<std::string, std::string> *unused);

        Status ConfigureOption(const ConfigOptions &config_options,
                               const std::string &name, const std::string &value);

        Status ConfigureFromString(const ConfigOptions &config_options,
                                   const std::string &opts);

        Status GetOptionString(const ConfigOptions &config_options,
                               std::string *result) const;

        std::string ToString(const ConfigOptions &config_options) const
        {
            return ToString(config_options, "");
        }
        std::string ToString(const ConfigOptions &config_options,
                             const std::string &prefix) const;

        Status GetOptionNames(const ConfigOptions &config_options,
                              std::unordered_set<std::string> *result) const;

        virtual Status GetOption(const ConfigOptions &config_options,
                                 const std::string &name, std::string *value) const;

        virtual bool AreEquivalent(const ConfigOptions &config_options,
                                   const Configurable *other,
                                   std::string *name) const;

        virtual std::string GetPrintableOptions() const { return ""; }

        virtual Status PrepareOptions(const ConfigOptions &config_options);

        virtual Status ValidateOptions(const DBOptions &db_opts,
                                       const ColumnFamilyOptions &cf_opts) const;

        static Status GetOptionsMap(
            const std::string &opt_value, const std::string &default_id,
            std::string *id, std::unordered_map<std::string, std::string> *options);

    protected:
        virtual const void *GetOptionsPtr(const std::string &name) const;

        virtual Status ParseStringOptions(const ConfigOptions &config_options,
                                          const std::string &opts_str);

        virtual Status ConfigureOptions(
            const ConfigOptions &config_options,
            const std::unordered_map<std::string, std::string> &opts_map,
            std::unordered_map<std::string, std::string> *unused);

        virtual Status ParseOption(const ConfigOptions &config_options,
                                   const OptionTypeInfo &opt_info,
                                   const std::string &opt_name,
                                   const std::string &opt_value, void *opt_ptr);

        virtual bool OptionsAreEqual(const ConfigOptions &config_options,
                                     const OptionTypeInfo &opt_info,
                                     const std::string &name,
                                     const void *const this_ptr,
                                     const void *const that_ptr,
                                     std::string *bad_name) const;

        virtual std::string SerializeOptions(const ConfigOptions &config_options,
                                             const std::string &header) const;

        virtual std::string GetOptionName(const std::string &long_name) const;

        template <typename T>
        void RegisterOptions(
            T *opt_ptr,
            const std::unordered_map<std::string, OptionTypeInfo> *opt_map)
        {
            RegisteredOptions(T::kName(), opt_ptr, opt_map);
        }
        void RegisterOptions(
            const std::string &name, void *opt_ptr,
            const std::unordered_map<std::string, OptionTypeInfo> *opt_map);

        inline bool HasRegisteredOptions() const { return !options_.empty(); }

    private:
        std::vector<RegisteredOptions> options_;
    };
}
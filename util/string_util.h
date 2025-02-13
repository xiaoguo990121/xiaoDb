#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    class Slice;

    std::vector<std::string> StringSplit(const std::string &arg, char delim);

    // Append a human-readable printout of "num" to *str
    void AppendNumberTo(std::string *str, uint64_t num);

    // Append a human-readable printout of "value" to *str.
    // Escapes any non-printable characters found in "value".
    void AppendEscapedStringTo(std::string *str, const Slice &value);

    // Put n digits from v in base kBase to (*buf)[0] to (*buf)[n-1] and
    // advance *buf to the position after what was written.
    template <size_t kBase>
    inline void PutBaseChars(char **buf, size_t n, uint64_t v, bool uppercase)
    {
        const char *digitChars = uppercase ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                           : "0123456789abcdefghijklmnopqrstuvwxyz";
        for (size_t i = n; i > 0; --i)
        {
            (*buf)[i - 1] = digitChars[static_cast<size_t>(v % kBase)];
            v /= kBase;
        }
        *buf += n;
    }

    // Parse n digits from *buf in base kBase to *v and advance *buf to the
    // position after what was read. On success, true is returned. On failure,
    // false is returned, *buf is placed at the first bad character, and *v
    // contains the partial parsed data. Overflow is not checked but the
    // result is accurate mod 2^64. Requires the starting value of *v to be
    // zero or previously accumulated parsed digits, i.e.
    //   ParseBaseChars(&b, n, &v);
    // is equivalent to n calls to
    //   ParseBaseChars(&b, 1, &v);
    template <int kBase>
    inline bool ParseBaseChars(const char **buf, size_t n, uint64_t *v)
    {
        while (n)
        {
            char c = **buf;
            *v *= static_cast<uint64_t>(kBase);
            if (c >= '0' && (kBase >= 10 ? c <= '9' : c < '0' + kBase))
            {
                *v += static_cast<uint64_t>(c - '0');
            }
            else if (kBase > 10 && c >= 'A' && c < 'A' + kBase - 10)
            {
                *v += static_cast<uint64_t>(c - 'A' + 10);
            }
            else if (kBase > 10 && c >= 'a' && c < 'a' + kBase - 10)
            {
                *v += static_cast<uint64_t>(c - 'a' + 10);
            }
            else
            {
                return false;
            }
            --n;
            ++*buf;
        }
        return true;
    }

    // Return a human-readable version of num.
    // for num >= 10.000, prints "xxK"
    // for num >= 10.000.000, prints "xxM"
    // for num >= 10.000.000.000, prints "xxG"
    std::string NumberToHumanString(int64_t num);

    std::string BytesToHumanString(uint64_t bytes);

    std::string TimeToHumanString(int unixtime);

    int AppendHumanMicros(uint64_t micros, char *output, int len,
                          bool fixed_format);

    int AppendHumanBytes(uint64_t bytes, char *output, int len);

    std::string EscapeString(const Slice &value);

    bool ConsumeDecimalNumber(Slice *in, uint64_t *val);

    bool isSpecialChar(const char c);

    char UnescapeChar(const char c);

    char EscapeChar(const char c);

    std::string EscapeOptionString(const std::string &raw_string);

    std::string UnescapeOptionString(const std::string &escaped_string);

    std::string trim(const std::string &str);

    bool EndsWith(const std::string &string, const std::string &pattern);

    bool StartsWith(const std::string &string, const std::string &pattern);

    bool ParseBoolean(const std::string &type, const std::string &value);

    uint8_t ParseUint8(const std::string &value);

    uint32_t ParseUint32(const std::string &value);

    int32_t ParseInt32(const std::string &value);

    uint64_t ParseUint64(const std::string &value);

    int ParseInt(const std::string &value);

    int64_t ParseInt64(const std::string &value);

    double ParseDouble(const std::string &value);

    size_t ParseSizeT(const std::string &value);

    std::vector<int> ParseVectorInt(const std::string &value);

    bool SerializeIntVector(const std::vector<int> &vec, std::string *value);

    int ParseTimeStringToSeconds(const std::string &value);

    bool TryParseTimeRangeString(const std::string &value, int &start_time,
                                 int &end_time);

    extern const std::string kNullptrString;

    std::string errnoStr(int err);

}
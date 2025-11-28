#pragma once

#include <cstdint>
#include <cstring>

namespace Jopa {

class IEEEdouble;
class IEEEfloat;

class Int
{
public:
    static constexpr int32_t MAX_INT() { return INT32_MAX; }
    static constexpr int32_t MIN_INT() { return INT32_MIN; }
};

class LongInt;
class ULongInt;

class BaseLong
{
protected:
    static constexpr uint32_t SHORT_MASK = 0xFFFF;
    static constexpr uint32_t SIGN_BIT = 0x80000000U;

    // Use a union to allow type punning between uint64_t and double
    // This is needed by IEEEdouble which inherits from BaseLong
    union {
        uint64_t words;
        double double_value;
    } value;

    // Protected setters for IEEEdouble to use
    void setHighWord(uint32_t high) {
        value.words = (uint64_t(high) << 32) | (value.words & 0xFFFFFFFF);
    }
    void setLowWord(uint32_t low) {
        value.words = (value.words & 0xFFFFFFFF00000000ULL) | low;
    }
    void setHighAndLowWords(uint32_t high, uint32_t low) {
        value.words = (uint64_t(high) << 32) | low;
    }
    void setHighAndLowWords(const BaseLong& op) {
        value.words = op.value.words;
    }

public:
    constexpr BaseLong() : value{0} {}
    constexpr BaseLong(uint32_t high, uint32_t low) : value{(uint64_t(high) << 32) | low} {}
    constexpr BaseLong(uint32_t a) : value{a} {}
    constexpr BaseLong(int32_t a) : value{uint64_t(int64_t(a))} {} // sign extends
    constexpr BaseLong(uint64_t a) : value{a} {}
    constexpr BaseLong(int64_t a) : value{uint64_t(a)} {}

    uint32_t HighWord() const { return uint32_t(value.words >> 32); }
    uint32_t LowWord() const { return uint32_t(value.words); }
    uint64_t Words() const { return value.words; }
    double DoubleView() const { return value.double_value; }

    BaseLong operator+(BaseLong op) const { return value.words + op.value.words; }
    BaseLong operator+() const { return *this; }
    BaseLong& operator+=(BaseLong op) { value.words += op.value.words; return *this; }
    BaseLong operator++(int) { BaseLong t = *this; ++value.words; return t; }
    BaseLong& operator++() { ++value.words; return *this; }

    BaseLong operator-(BaseLong op) const { return value.words - op.value.words; }
    BaseLong operator-() const { return uint64_t(-int64_t(value.words)); }
    BaseLong& operator-=(BaseLong op) { value.words -= op.value.words; return *this; }
    BaseLong operator--(int) { BaseLong t = *this; --value.words; return t; }
    BaseLong& operator--() { --value.words; return *this; }

    BaseLong operator*(BaseLong op) const { return value.words * op.value.words; }
    BaseLong& operator*=(BaseLong op) { value.words *= op.value.words; return *this; }

    // NOTE: To match the JLS, mask the argument with 0x3f
    BaseLong operator<<(int op) const { return value.words << op; }
    BaseLong& operator<<=(int op) { value.words <<= op; return *this; }

    bool operator==(BaseLong op) const { return value.words == op.value.words; }
    bool operator!=(BaseLong op) const { return value.words != op.value.words; }
    bool operator!() const { return !value.words; }

    BaseLong operator~() const { return ~value.words; }
    BaseLong operator^(BaseLong op) const { return value.words ^ op.value.words; }
    BaseLong& operator^=(BaseLong op) { value.words ^= op.value.words; return *this; }
    BaseLong operator|(BaseLong op) const { return value.words | op.value.words; }
    BaseLong& operator|=(BaseLong op) { value.words |= op.value.words; return *this; }
    BaseLong operator&(BaseLong op) const { return value.words & op.value.words; }
    BaseLong& operator&=(BaseLong op) { value.words &= op.value.words; return *this; }

    bool operator&&(BaseLong op) const { return value.words && op.value.words; }
    bool operator||(BaseLong op) const { return value.words || op.value.words; }

    operator LongInt() const;
    operator ULongInt() const;

    int32_t hashCode() const { return int32_t(HighWord() ^ LowWord()); }
};


class LongInt : public BaseLong
{
public:
    constexpr LongInt() : BaseLong() {}
    constexpr LongInt(uint32_t high, uint32_t low) : BaseLong(high, low) {}
    constexpr LongInt(uint32_t a) : BaseLong(a) {}
    constexpr LongInt(int32_t a) : BaseLong(a) {}
    constexpr LongInt(uint64_t a) : BaseLong(a) {}
    constexpr LongInt(int64_t a) : BaseLong(a) {}
    explicit LongInt(const BaseLong& a) : BaseLong(a) {}
    explicit LongInt(const IEEEdouble&);
    explicit LongInt(const IEEEfloat&);

    static constexpr LongInt MAX_LONG() { return LongInt(INT64_MAX); }
    static constexpr LongInt MIN_LONG() { return LongInt(INT64_MIN); }
    static void ConstantCleanup() {} // no-op now

    LongInt operator/(LongInt op) const { return int64_t(value.words) / int64_t(op.value.words); }
    LongInt& operator/=(LongInt op) { value.words = uint64_t(int64_t(value.words) / int64_t(op.value.words)); return *this; }

    LongInt operator%(LongInt op) const { return int64_t(value.words) % int64_t(op.value.words); }
    LongInt& operator%=(LongInt op) { value.words = uint64_t(int64_t(value.words) % int64_t(op.value.words)); return *this; }

    // Arithmetic (signed) right shift
    LongInt operator>>(int op) const { return int64_t(value.words) >> op; }
    LongInt& operator>>=(int op) { value.words = uint64_t(int64_t(value.words) >> op); return *this; }

    bool operator<(LongInt op) const { return int64_t(value.words) < int64_t(op.value.words); }
    bool operator>(LongInt op) const { return int64_t(value.words) > int64_t(op.value.words); }
    bool operator<=(LongInt op) const { return int64_t(value.words) <= int64_t(op.value.words); }
    bool operator>=(LongInt op) const { return int64_t(value.words) >= int64_t(op.value.words); }
};


class ULongInt : public BaseLong
{
public:
    constexpr ULongInt() : BaseLong() {}
    constexpr ULongInt(uint32_t high, uint32_t low) : BaseLong(high, low) {}
    constexpr ULongInt(uint32_t a) : BaseLong(a) {}
    constexpr ULongInt(int32_t a) : BaseLong(a) {}
    constexpr ULongInt(uint64_t a) : BaseLong(a) {}
    constexpr ULongInt(int64_t a) : BaseLong(a) {}

    ULongInt operator/(ULongInt op) const { return value.words / op.value.words; }
    ULongInt& operator/=(ULongInt op) { value.words /= op.value.words; return *this; }

    ULongInt operator%(ULongInt op) const { return value.words % op.value.words; }
    ULongInt& operator%=(ULongInt op) { value.words %= op.value.words; return *this; }

    // Logical (unsigned) right shift
    ULongInt operator>>(int op) const { return value.words >> op; }
    ULongInt& operator>>=(int op) { value.words >>= op; return *this; }

    bool operator<(ULongInt op) const { return value.words < op.value.words; }
    bool operator>(ULongInt op) const { return value.words > op.value.words; }
    bool operator<=(ULongInt op) const { return value.words <= op.value.words; }
    bool operator>=(ULongInt op) const { return value.words >= op.value.words; }
};


inline BaseLong::operator LongInt() const { return LongInt(value.words); }
inline BaseLong::operator ULongInt() const { return ULongInt(value.words); }

} // namespace Jopa

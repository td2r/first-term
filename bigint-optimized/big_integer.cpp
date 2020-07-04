#include "big_integer.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <sstream>

namespace {
    using storage_t = vector;

    uint64_t const base = static_cast<uint64_t>(UINT32_MAX) + 1;

    // r = a * b
    // r == a also works
    void multiply(storage_t const& a, uint32_t b, storage_t& r, uint64_t carry = 0) {
        auto b64 = static_cast<uint64_t>(b);
        r.resize(a.size());
        for (size_t i = 0; i < a.size(); ++i) {
            carry = carry + a[i] * b64;
            r[i] = static_cast<uint32_t>(carry);
            carry >>= 32U;
        }
        if (carry) {
            r.push_back(carry);
        }
    }

    // a /= x
    // return a % x
    uint32_t divide(storage_t& a, uint32_t x) {
        uint64_t carry = 0;
        for (size_t i = a.size(); i-->0;) {
            carry = ((carry << 32U) | a[i]);
            a[i] = carry / x;
            carry = carry - a[i] * x;
        }
        return carry;
    }

    uint32_t trial(__uint128_t r1, __uint128_t r2, __uint128_t r3, __uint128_t d2) {
        return std::min(static_cast<uint32_t>(((((r1 << 32U) | r2) << 32U) | r3) / d2), UINT32_MAX);
    }

    bool smaller(storage_t const& r, storage_t const& dq, size_t k, size_t m) {
        for (size_t i = m + 1; i-->0;) {
            if (r[i + k] != dq[i]) {
                return r[i + k] < dq[i];
            }
        }
        return false;
    }

    bool less(storage_t const& a, storage_t const& b) {
        size_t n = a.size();
        size_t m = b.size();
        return n == m ? smaller(a, b, 0, n - 1) : n < m;
    }

    void difference(storage_t& r, storage_t const& dq, size_t k, size_t m) {
        uint64_t borrow = 0;
        for (size_t i = 0; i <= m; ++i) {
            uint64_t diff = r[k + i] - (dq[i] + borrow);
            borrow = (r[k + i] < dq[i] + borrow);
            r[k + i] = diff;
        }
    }

    auto const bit_and = [](uint32_t x, uint32_t y) {return x & y;};
    auto const bit_or  = [](uint32_t x, uint32_t y) {return x | y;};
    auto const bit_xor = [](uint32_t x, uint32_t y) {return x ^ y;};
}

big_integer::big_integer() : sign_(0), words_() {}

big_integer::big_integer(big_integer const& other) = default;

big_integer::big_integer(int a) : sign_((a > 0) - (a < 0)), words_() {
    if (a != -1 && a != 0) {
        words_.push_back(static_cast<uint32_t>(a));
    }
}

big_integer::big_integer(std::string const& str) : sign_(1) {
    bool negative = false;
    size_t i = 0;
    if (str.length() != 0 && str[0] == '-') {
        negative = true;
        ++i;
    }
    while (i < str.length()) {
        multiply(words_, 10, words_, str[i] - '0');
        ++i;
    }
    if (negative) {
        *this = -*this;
    } else {
        shrink_words_();
    }
}

big_integer::~big_integer() = default;

big_integer& big_integer::operator=(big_integer const& other) {
    if (this != &other) {
        big_integer tmp(other);
        swap_(tmp);
    }
    return *this;
}

big_integer& big_integer::operator+=(const big_integer& rhs) {
    uint64_t carry = 0;
    resize_(std::max(words_.size(), rhs.words_.size()) + 1);
    for (size_t i = 0; i < words_.size(); ++i) {
        carry = carry + words_[i] + rhs.s_digit_(i);
        words_[i] = static_cast<uint32_t>(carry);
        carry >>= 32U;
    }
    sign_ = (words_.back() >> 31U) == 0 ? 1 : -1;
    shrink_words_();
    return *this;
}

big_integer& big_integer::operator-=(const big_integer& rhs) {
    return *this += -rhs;
}

big_integer& big_integer::operator*=(const big_integer& rhs) {
    big_integer const& a(sign_ == -1 ? -*this : *this);
    big_integer const& b(rhs.sign_ == -1 ? -rhs : rhs);
    big_integer product;
    product.sign_ = 1;
    int8_t result_sign = sign_ * rhs.sign_;
    product.resize_(a.words_.size() + b.words_.size() + 1);
    for (size_t i = 0; i < a.words_.size(); ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < b.words_.size() || carry != 0; ++j) {
            carry = carry + static_cast<uint64_t>(a.words_[i]) * b.s_digit_(j) + product.words_[i + j];
            product.words_[i + j] = static_cast<uint32_t>(carry);
            carry >>= 32U;
        }
    }
    if (result_sign == -1) {
        product = -product;
    } else {
        product.shrink_words_();
    }
    swap_(product);
    return *this;
}

big_integer& big_integer::operator/=(const big_integer& rhs) {
    int8_t result_sign = sign_ / rhs.sign_;
    if (sign_ == -1) *this = -*this;
    storage_t d = (rhs.sign_ == -1 ? -rhs : rhs).words_;
    if (less(words_, d)) {
        return *this = 0;
    } else if (d.size() == 1) {
        sign_ = 1;
        divide(words_, d[0]);
    } else {
        uint32_t f = base / (static_cast<uint64_t>(d.back()) + 1);
        multiply(words_, f, words_);
        multiply(d, f, d);

        size_t n = words_.size() - (words_.back() == 0);
        size_t m = d.size() - (d.back() == 0);
        words_.resize(n + 1);
        d.resize(m + 1);

        uint64_t d2 = ((static_cast<uint64_t>(d[m - 1]) << 32U) | d[m - 2]);
        storage_t q(n - m + 1);
        storage_t dq(d);

        for (uint32_t i = 0; i <= n - m; ++i) {
            uint32_t k = n - m - i;
            uint32_t qt = trial(words_[m + k], words_[m + k - 1], words_[m + k - 2], d2);
            multiply(d, qt, dq);
            if (smaller(words_, dq, k, m)) {
                --qt;
                difference(dq, d, 0, m);
            }
            q[k] = qt;
            difference(words_, dq, k, m);
        }
        words_.swap(q);
    }
    if (result_sign == -1) {
        *this = -*this;
    } else {
        shrink_words_();
    }
    return *this;
}

big_integer& big_integer::operator%=(const big_integer& rhs) {
    *this -= (*this / rhs) * rhs;
    return *this;
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
    apply_bitwise_(bit_and, rhs);
    return *this;
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
    apply_bitwise_(bit_or, rhs);
    return *this;
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
    apply_bitwise_(bit_xor, rhs);
    return *this;
}

big_integer& big_integer::operator<<=(int rhs) {
    size_t i = 0;
    auto s = static_cast<uint32_t>(rhs);
    if (s >= 32) {
        i = s / 32;
        std::reverse(words_.begin(), words_.end());
        words_.resize(words_.size() + i);
        std::reverse(words_.begin(), words_.end());
        s %= 32;
    }
    if (s != 0) {
        uint64_t carry = 0;
        resize_(words_.size() + 1);
        for (; i < words_.size(); ++i) {
            carry |= (static_cast<uint64_t>(words_[i]) << s);
            words_[i] = static_cast<uint32_t>(carry);
            carry >>= 32U;
        }
        shrink_words_();
    }
    return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
    auto s = static_cast<uint32_t>(rhs);
    if (s >= 32) {
        size_t rm = s / 32;
        if (rm >= words_.size()) {
            words_.clear();
        } else {
            words_.erase(words_.begin(), words_.begin() + rm);
        }
        s %= 32;
    }
    if (s != 0) {
        uint32_t mask = (1U << s) - 1;
        uint64_t carry = (sign_ == -1 ? mask : 0);
        for (size_t i = words_.size(); i --> 0;) {
            carry = ((carry << 32U) | words_[i]);
            words_[i] = (carry >> s);
            carry &= mask;
        }
        shrink_words_();
    }
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    return sign_ == 0 ? *this : (~(*this) + 1);
}

big_integer big_integer::operator~() const {
    big_integer result(*this);
    for (size_t i = 0; i < words_.size(); ++i) {
        result.words_[i] = ~words_[i];
    }
    result.sign_ = -sign_;
    return result;
}

big_integer& big_integer::operator++() {
    return *this += 1;
}

big_integer big_integer::operator++(int) {
    big_integer tmp(*this + 1);
    swap_(tmp);
    return tmp;
}

big_integer& big_integer::operator--() {
    return *this -= 1;
}

big_integer big_integer::operator--(int) {
    big_integer tmp(*this - 1);
    swap_(tmp);
    return tmp;
}

bool operator==(big_integer const& a, big_integer const& b) {
    return a.sign_ == b.sign_ && a.words_ == b.words_;
}

bool operator!=(big_integer const& a, big_integer const& b) {
    return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b) {
    if (a.sign_ != b.sign_) {
        return a.sign_ < b.sign_;
    }
    return less(a.words_, b.words_);
}

bool operator>(big_integer const& a, big_integer const& b) {
    if (a.sign_ != b.sign_) {
        return a.sign_ > b.sign_;
    }
    return less(b.words_, a.words_);
}

bool operator<=(big_integer const& a, big_integer const& b) {
    return !(a > b);
}

bool operator>=(big_integer const& a, big_integer const& b) {
    return !(a < b);
}

std::string to_string(big_integer const& a) {
    if (a.sign_ == 0) {
        return "0";
    }
    std::stringstream ss;
    if (a.sign_ == -1) {
        ss << '-';
    }
    storage_t words = (a.sign_ == -1 ? -a : a).words_;
    std::vector<char> digits;
    while (!words.empty()) {
        digits.push_back(divide(words, 10) + '0');
        while (!words.empty() && words.back() == 0) {
            words.pop_back();
        }
    }
    for (size_t i = digits.size(); i --> 0;) {
        ss << digits[i];
    }
    return ss.str();
}

void big_integer::shrink_words_() {
    uint32_t const useless = (sign_ == -1) * UINT32_MAX;
    while (!words_.empty() && words_.back() == useless) {
        words_.pop_back();
    }
    if (words_.empty() && sign_ == 1) {
        sign_ = 0;
    }
}

void big_integer::swap_(big_integer& rhs) {
    std::swap(sign_, rhs.sign_);
    words_.swap(rhs.words_);
}

void big_integer::resize_(size_t n) {
    words_.resize(n, (sign_ == -1) * UINT32_MAX);
}

void big_integer::apply_bitwise_(const std::function<uint32_t (uint32_t, uint32_t)>& f, big_integer const& rhs) {
    resize_(std::max(words_.size(), rhs.words_.size()) + 1);
    for (size_t i = 0; i < words_.size(); ++i) {
        words_[i] = f(words_[i], rhs.s_digit_(i));
    }
    sign_ = (words_.back() >> 31U) == 0 ? 1 : -1;
    shrink_words_();
}

uint32_t big_integer::s_digit_(size_t i) const {
    return i < words_.size() ? words_[i] : (sign_ == -1) * UINT32_MAX;
}

big_integer operator+(big_integer a, big_integer const& b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const& b) {
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b) {
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b) {
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
    return a %= b;
}

big_integer operator&(big_integer a, big_integer const& b) {
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b) {
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b) {
    return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
    return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
    return a >>= b;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
    return s << to_string(a);
}

#pragma once

#include <gmpxx.h>
#include <string>
#include <utility>

class BigInt {
    mpz_class data;

    template<typename... T>
    BigInt(T &&...args)
        : data(std::forward<T>(args)...) {}

public:
    static BigInt from_string(std::string_view str) {
        return BigInt(std::string(str));
    }

    std::string to_string() const {
        return data.get_str();
    }

    operator bool() const {
        return data != 0;
    }

    friend BigInt operator+(BigInt const &lval, BigInt const &rval) {
        return BigInt(lval.data + rval.data);
    }

    friend BigInt operator-(BigInt const &lval, BigInt const &rval) {
        return BigInt(lval.data - rval.data);
    }

    friend BigInt operator*(BigInt const &lval, BigInt const &rval) {
        return BigInt(lval.data * rval.data);
    }

    friend BigInt operator/(BigInt const &lval, BigInt const &rval) {
        return BigInt(lval.data / rval.data);
    }

    friend BigInt operator%(BigInt const &lval, BigInt const &rval) {
        return BigInt(lval.data % rval.data);
    }

    friend bool operator>(BigInt const &lval, BigInt const &rval) {
        return lval.data > rval.data;
    }

    friend bool operator<(BigInt const &lval, BigInt const &rval) {
        return lval.data < rval.data;
    }

    friend bool operator>=(BigInt const &lval, BigInt const &rval) {
        return lval.data >= rval.data;
    }

    friend bool operator<=(BigInt const &lval, BigInt const &rval) {
        return lval.data <= rval.data;
    }

    friend bool operator==(BigInt const &lval, BigInt const &rval) {
        return lval.data == rval.data;
    }

    friend bool operator!=(BigInt const &lval, BigInt const &rval) {
        return lval.data != rval.data;
    }
};

#pragma once
#include <gmpxx.h>
#include <utility>
#include <string>
class StrInt {
    mpz_class data;
    template <typename... T>
    StrInt(T &&...args):
        data(std::forward<T>(args)...) {}
public:
    static StrInt from_string(std::string const &str) {
        return StrInt(str);
    }
    std::string to_string() const {
        return data.get_str();
    }
    operator bool() const {
        return data != 0;
    }
    friend StrInt operator+(StrInt const &lval, StrInt const &rval) {
        return StrInt(lval.data + rval.data);
    }
    friend StrInt operator-(StrInt const &lval, StrInt const &rval) {
        return StrInt(lval.data - rval.data);
    }
    friend StrInt operator*(StrInt const &lval, StrInt const &rval) {
        return StrInt(lval.data * rval.data);
    }
    friend StrInt operator/(StrInt const &lval, StrInt const &rval) {
        return StrInt(lval.data / rval.data);
    }
    friend StrInt operator%(StrInt const &lval, StrInt const &rval) {
        return StrInt(lval.data % rval.data);
    }
    friend bool operator>(StrInt const &lval, StrInt const &rval) {
        return lval.data > rval.data;
    }
    friend bool operator<(StrInt const &lval, StrInt const &rval) {
        return lval.data < rval.data;
    }
    friend bool operator>=(StrInt const &lval, StrInt const &rval) {
        return lval.data >= rval.data;
    }
    friend bool operator<=(StrInt const &lval, StrInt const &rval) {
        return lval.data <= rval.data;
    }
    friend bool operator==(StrInt const &lval, StrInt const &rval) {
        return lval.data == rval.data;
    }
    friend bool operator!=(StrInt const &lval, StrInt const &rval) {
        return lval.data != rval.data;
    }
};

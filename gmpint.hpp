#pragma once
#include <gmpxx.h>
#include <utility>
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
    friend inline StrInt operator+(StrInt const &, StrInt const &);
    friend inline StrInt operator-(StrInt const &, StrInt const &);
    friend inline StrInt operator*(StrInt const &, StrInt const &);
    friend inline StrInt operator/(StrInt const &, StrInt const &);
    friend inline StrInt operator%(StrInt const &, StrInt const &);
    friend inline bool operator==(StrInt const &, StrInt const &);
    friend inline bool operator!=(StrInt const &, StrInt const &);
    friend inline bool operator<=(StrInt const &, StrInt const &);
    friend inline bool operator>=(StrInt const &, StrInt const &);
    friend inline bool operator<(StrInt const &, StrInt const &);
    friend inline bool operator>(StrInt const &, StrInt const &);
};
StrInt operator+(StrInt const &lval, StrInt const &rval) {
    return StrInt(lval.data + rval.data);
}
StrInt operator-(StrInt const &lval, StrInt const &rval) {
    return StrInt(lval.data - rval.data);
}
StrInt operator*(StrInt const &lval, StrInt const &rval) {
    return StrInt(lval.data * rval.data);
}
StrInt operator/(StrInt const &lval, StrInt const &rval) {
    return StrInt(lval.data / rval.data);
}
StrInt operator%(StrInt const &lval, StrInt const &rval) {
    return StrInt(lval.data % rval.data);
}
bool operator==(StrInt const &lval, StrInt const &rval) {
    return lval.data == rval.data;
}
bool operator!=(StrInt const &lval, StrInt const &rval) {
    return lval.data != rval.data;
}
bool operator<(StrInt const &lval, StrInt const &rval) {
    return lval.data < rval.data;
}
bool operator>(StrInt const &lval, StrInt const &rval) {
    return lval.data > rval.data;
}
bool operator<=(StrInt const &lval, StrInt const &rval) {
    return lval.data <= rval.data;
}
bool operator>=(StrInt const &lval, StrInt const &rval) {
    return lval.data >= rval.data;
}

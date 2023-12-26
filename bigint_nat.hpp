#pragma once
#include <stdint.h>
#include <string>
#include <stdexcept>
class BigInt {
    size_t len;
    int8_t const *arr;
    size_t *ctr;
    int8_t get(size_t i) const {
        return arr[i < len ? i : len];
    }
    BigInt(size_t rlen, int8_t const *rarr):
        len(rlen), arr(rarr), ctr(new size_t(1)) {
        while (len && arr[len - 1] == arr[len]) {
            len--;
        }
    }
public:
    static BigInt from_string(std::string_view sv) {
        auto itr = sv.rbegin(), end = sv.rend();
        if (sv.front() == '+' || sv.front() == '-') {
            end--;
        }
        size_t len = end - itr;
        int8_t *arr = new int8_t[len + 1];
        if (sv.front() == '-') {
            int8_t d = 10;
            for (size_t j = 0; itr != end; itr++) {
                if (*itr < '0' || *itr > '9') {
                    delete[] arr;
                    throw std::invalid_argument("invalid argument");
                } else {
                    d = '9' - *itr + (d == 10);
                    arr[j++] = d == 10 ? 0 : d;
                }
            }
            arr[len] = d == 10 ? 0 : 9;
        } else {
            for (size_t j = 0; itr != end; itr++) {
                if (*itr < '0' || *itr > '9') {
                    delete[] arr;
                    throw std::invalid_argument("invalid argument");
                } else {
                    arr[j++] = *itr - '0';
                }
            }
            arr[len] = 0;
        }
        return BigInt(len, arr);
    }
    std::string to_string() const {
        char str[len + 2]; // GCC and Clang variable length array extension
        char *itr = &str[len + 2], *end = &str[len + 2];
        if (arr[len]) {
            bool flag = true;
            for (size_t i = 0; i < len; i++) {
                int8_t d = '9' - arr[i] + flag;
                *--itr = (flag = d > '9') ? '0' : d;
            }
            if (flag) {
                *--itr = '1';
                *--itr = '-';
            } else {
                *--itr = '-';
            }
        } else if (len) {
            for (size_t i = 0; i < len; i++) {
                *--itr = '0' + arr[i];
            }
        } else {
            *--itr = '0';
        }
        return std::string(itr, end);
    }
    BigInt(BigInt const &rval):
        len(rval.len), arr(rval.arr), ctr(rval.ctr) {
        ++*ctr;
    }
    BigInt &operator=(BigInt const &rval) {
        ++*rval.ctr;
        if (--*ctr == 0) {
            delete[] arr;
            delete ctr;
        }
        len = rval.len;
        arr = rval.arr;
        ctr = rval.ctr;
        return *this;
    }
    ~BigInt() {
        if (--*ctr == 0) {
            delete[] arr;
            delete ctr;
        }
    }
    operator bool() const {
        return len || arr[len];
    }
    friend BigInt operator+(BigInt const &lbi, BigInt const &rbi) {
        size_t len = (lbi.len > rbi.len ? lbi.len : rbi.len) + 1;
        int8_t *arr = new int8_t[len + 1];
        int8_t s = 0;
        for (size_t i = 0; i <= len; i++) {
            s = lbi.get(i) + rbi.get(i) + (s >= 10);
            arr[i] = s >= 10 ? s - 10 : s;
        }
        return BigInt(len, arr);
    }
    friend BigInt operator-(BigInt const &lbi, BigInt const &rbi) {
        size_t len = (lbi.len > rbi.len ? lbi.len : rbi.len) + 1;
        int8_t *arr = new int8_t[len + 1];
        int8_t d = 0;
        for (size_t i = 0; i <= len; i++) {
            d = lbi.get(i) - rbi.get(i) - (d < 0);
            arr[i] = d < 0 ? d + 10 : d;
        }
        return BigInt(len, arr);
    }
    friend BigInt operator*(BigInt const &lbi, BigInt const &rbi) {
        size_t len = lbi.len + rbi.len + 1;
        int8_t *arr = new int8_t[len + 1]();
        for (size_t i = 0; i <= len; i++) {
            int8_t p = 0, s = 0;
            for (size_t j = 0; i + j <= len; j++) {
                p = lbi.get(j) * rbi.get(i) + p / 10;
                s = p % 10 + arr[i + j] + (s >= 10);
                arr[i + j] = s >= 10 ? s - 10 : s;
            }
        }
        return BigInt(len, arr);
    }
    template <bool select>
    friend BigInt divmod(BigInt const &lbi, BigInt const &rbi) {
        size_t len = lbi.len + rbi.len;
        int8_t *parr = new int8_t[len + 1], *narr = new int8_t[len + 1];
        int8_t *qarr = new int8_t[lbi.len + 1];
        int8_t *rarr = new int8_t[rbi.len + 1];
        for (size_t i = 0; i <= len; i++) {
            parr[i] = lbi.get(i);
        }
        if (lbi.arr[lbi.len] == rbi.arr[rbi.len]) {
            for (size_t i = lbi.len; i <= lbi.len; i--) {
                for (qarr[i] = 0;; qarr[i]++) {
                    int8_t d = 0;
                    for (size_t j = 0; i + j <= len; j++) {
                        d = parr[i + j] - rbi.get(j) - (d < 0);
                        narr[i + j] = d < 0 ? d + 10 : d;
                    }
                    if (narr[len] != parr[len]) {
                        break;
                    }
                    for (size_t j = 0; i + j <= len; j++) {
                        parr[i + j] = narr[i + j];
                    }
                }
            }
            for (size_t i = 0; i <= rbi.len; i++) {
                rarr[i] = parr[i];
            }
        } else {
            for (size_t i = lbi.len; i <= lbi.len; i--) {
                for (qarr[i] = 9;; qarr[i]--) {
                    int8_t s = 0;
                    for (size_t j = 0; i + j <= len; j++) {
                        s = parr[i + j] + rbi.get(j) + (s >= 10);
                        narr[i + j] = s >= 10 ? s - 10 : s;
                    }
                    if (narr[len] != parr[len]) {
                        break;
                    }
                    for (size_t j = 0; i + j <= len; j++) {
                        parr[i + j] = narr[i + j];
                    }
                }
            }
            for (size_t i = 0; i <= rbi.len; i++) {
                rarr[i] = narr[i];
            }
        }
        delete[] parr;
        delete[] narr;
        if constexpr (select) {
            delete[] qarr;
            return BigInt(rbi.len, rarr);
        } else {
            delete[] rarr;
            return BigInt(lbi.len, qarr);
        }
    }
    friend BigInt operator/(BigInt const &lbi, BigInt const &rbi) {
        return divmod<0>(lbi, rbi);
    }
    friend BigInt operator%(BigInt const &lbi, BigInt const &rbi) {
        return divmod<1>(lbi, rbi);
    }
    template <auto gt, auto eq, auto lt>
    friend auto compare(BigInt const &lbi, BigInt const &rbi) {
        if (lbi.arr[lbi.len] < rbi.arr[rbi.len]) {
            return gt;
        }
        if (lbi.arr[lbi.len] > rbi.arr[rbi.len]) {
            return lt;
        }
        for (size_t m = lbi.len > rbi.len ? lbi.len : rbi.len, i = m - 1; i < m; i--) {
            if (lbi.get(i) > rbi.get(i)) {
                return gt;
            }
            if (lbi.get(i) < rbi.get(i)) {
                return lt;
            }
        }
        return eq;
    }
    friend bool operator>(BigInt const &lbi, BigInt const &rbi) {
        return compare<true, false, false>(lbi, rbi);
    }
    friend bool operator<(BigInt const &lbi, BigInt const &rbi) {
        return compare<false, false, true>(lbi, rbi);
    }
    friend bool operator>=(BigInt const &lbi, BigInt const &rbi) {
        return compare<true, true, false>(lbi, rbi);
    }
    friend bool operator<=(BigInt const &lbi, BigInt const &rbi) {
        return compare<false, true, true>(lbi, rbi);
    }
    friend bool operator==(BigInt const &lbi, BigInt const &rbi) {
        return compare<false, true, false>(lbi, rbi);
    }
    friend bool operator!=(BigInt const &lbi, BigInt const &rbi) {
        return compare<true, false, true>(lbi, rbi);
    }
};

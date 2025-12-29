#pragma once

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <gmp.h>

class BigInt {
    mpz_t data;

    BigInt() {
        mpz_init(data);
    }

    explicit BigInt(mpz_t const &value) {
        mpz_init_set(data, value);
    }

    static BigInt from_op_result(auto op) {
        BigInt out;
        op(out.data);
        return out;
    }

public:
    BigInt(BigInt const &other) {
        mpz_init_set(data, other.data);
    }

    BigInt(BigInt &&other) noexcept {
        mpz_init(data);
        mpz_swap(data, other.data);
    }

    BigInt &operator=(BigInt other) noexcept {
        mpz_swap(data, other.data);
        return *this;
    }

    ~BigInt() {
        mpz_clear(data);
    }

    operator bool() const {
        return mpz_sgn(data) != 0;
    }

    friend BigInt operator+(BigInt const &lval, BigInt const &rval) {
        return from_op_result([&](mpz_t out) { mpz_add(out, lval.data, rval.data); });
    }

    friend BigInt operator-(BigInt const &lval, BigInt const &rval) {
        return from_op_result([&](mpz_t out) { mpz_sub(out, lval.data, rval.data); });
    }

    friend BigInt operator*(BigInt const &lval, BigInt const &rval) {
        return from_op_result([&](mpz_t out) { mpz_mul(out, lval.data, rval.data); });
    }

    friend BigInt operator/(BigInt const &lval, BigInt const &rval) {
        return from_op_result([&](mpz_t out) { mpz_tdiv_q(out, lval.data, rval.data); });
    }

    friend BigInt operator%(BigInt const &lval, BigInt const &rval) {
        return from_op_result([&](mpz_t out) { mpz_tdiv_r(out, lval.data, rval.data); });
    }

    friend bool operator>(BigInt const &lval, BigInt const &rval) {
        return mpz_cmp(lval.data, rval.data) > 0;
    }

    friend bool operator<(BigInt const &lval, BigInt const &rval) {
        return mpz_cmp(lval.data, rval.data) < 0;
    }

    friend bool operator>=(BigInt const &lval, BigInt const &rval) {
        return mpz_cmp(lval.data, rval.data) >= 0;
    }

    friend bool operator<=(BigInt const &lval, BigInt const &rval) {
        return mpz_cmp(lval.data, rval.data) <= 0;
    }

    friend bool operator==(BigInt const &lval, BigInt const &rval) {
        return mpz_cmp(lval.data, rval.data) == 0;
    }

    friend bool operator!=(BigInt const &lval, BigInt const &rval) {
        return mpz_cmp(lval.data, rval.data) != 0;
    }

    static BigInt from_string(std::string_view str) {
        BigInt out;
        // mpz_set_str returns 0 on success, -1 on invalid input.
        if (mpz_set_str(out.data, std::string(str).c_str(), 10) != 0) {
            throw std::invalid_argument("invalid argument");
        }
        return out;
    }

    std::string to_string() const {
        char *raw = mpz_get_str(nullptr, 10, data);
        if (raw == nullptr) {
            throw std::runtime_error("mpz_get_str failed");
        }
        std::string out(raw);
        std::free(raw);
        return out;
    }
};

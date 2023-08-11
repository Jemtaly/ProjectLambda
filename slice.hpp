#pragma once
#include <istream>
#include <memory>
#include <string>
class Slice {
    std::shared_ptr<char const []> arr;
    char const *pos;
    char const *end;
    Slice(std::shared_ptr<char const []> const &arr, char const *pos, char const *end): arr(arr), pos(pos), end(end) {}
public:
    static Slice getline(std::istream &is) {
        std::size_t cap = 16;
        std::size_t len = 0;
        auto arr = std::make_shared<char []>(cap);
        char *pos = &arr[0x0];
        char *end = &arr[len];
        for (char c; is.get(c) && c != '\n'; len++, *end++ = c) {
            if (len == cap) {
                cap *= 2;
                auto tmp = std::make_shared<char []>(cap);
                std::copy(pos, end, tmp.get());
                arr = tmp;
                pos = &arr[0x0];
                end = &arr[len];
            }
        }
        return Slice(arr, pos, end);
    }
    operator std::string() const {
        return std::string(pos, end);
    }
    bool operator==(char const *str) const {
        return std::equal(pos, end, str);
    }
    bool empty() const {
        return pos == end;
    }
    size_t size() const {
        return end - pos;
    }
    Slice operator()(std::ptrdiff_t add, std::ptrdiff_t sub) const {
        return Slice(arr, (add < 0 ? end : pos) + add, (sub > 0 ? pos : end) + sub);
    }
    char operator[](std::ptrdiff_t idx) const {
        return (idx < 0 ? end : pos)[idx];
    }
};

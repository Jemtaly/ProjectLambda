#pragma once
#include <istream>
#include <memory>
#include <string>
class Slice {
    std::shared_ptr<char const []> arr;
    char const *beg;
    char const *end;
    Slice(std::shared_ptr<char const []> const &arr, char const *beg, char const *end): arr(arr), beg(beg), end(end) {}
public:
    static Slice getline(std::istream &is) {
        std::size_t cap = 16;
        std::size_t len = 0;
        auto arr = std::make_shared<char []>(cap);
        char *beg = &arr[0x0];
        char *end = &arr[len];
        for (char c; is.get(c) && c != '\n'; len++, *end++ = c) {
            if (len == cap) {
                cap *= 2;
                auto tmp = std::make_shared<char []>(cap);
                std::copy(beg, end, tmp.get());
                arr = tmp;
                beg = &arr[0x0];
                end = &arr[len];
            }
        }
        return Slice(arr, beg, end);
    }
    operator std::string() const {
        return std::string(beg, end);
    }
    operator std::string_view() const {
        return std::string_view(beg, end - beg);
    }
    bool operator==(char const *str) const {
        return std::equal(beg, end, str);
    }
    bool empty() const {
        return beg == end;
    }
    size_t size() const {
        return end - beg;
    }
    char operator[](std::ptrdiff_t idx) const {
        return (idx < 0 ? end : beg)[idx];
    }
    Slice operator()(std::ptrdiff_t add, std::ptrdiff_t sub) const {
        return Slice(arr, (add < 0 ? end : beg) + add, (sub > 0 ? beg : end) + sub);
    }
    char const *get_beg() const {
        return beg;
    }
    char const *get_end() const {
        return end;
    }
    Slice from_to(char const *beg, char const *end) const {
        return Slice(arr, beg, end);
    }
    void reset_to(char const *beg, char const *end) {
        this->beg = beg;
        this->end = end;
    }
};

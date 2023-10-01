#pragma once
#include <utility>
#include <cassert>
// The difference between Opt and Box is that Opt can be initialized or
// assigned to nullptr, while Box can only be null after being moved.
// Besides, the data in const Opt can still be modified, while the data
// in const Box cannot be modified.
// Both Opt and Box can be copied and moved. The data in them will also
// be copied when they are copied.
template <typename T>
class Opt {
    T *data;
    Opt(T *data):
        data(data) {}
public:
    Opt(std::nullptr_t):
        data(nullptr) {}
    Opt(Opt const &other):
        data(other.data ? new T(*other.data) : nullptr) {}
    Opt(Opt &&other):
        data(other.data) {
        other.data = nullptr;
    }
    Opt &operator=(std::nullptr_t) {
        delete data;
        data = nullptr;
        return *this;
    }
    Opt &operator=(Opt const &other) {
        if (this != &other) {
            delete data;
            data = other.data ? new T(*other.data) : nullptr;
        }
        return *this;
    }
    Opt &operator=(Opt &&other) {
        assert(this != &other); // self-assignment is not allowed
        delete data;
        data = other.data;
        other.data = nullptr;
        return *this;
    }
    ~Opt() {
        delete data;
    }
    template <typename... Args>
    static Opt make(Args &&...args) {
        return Opt(new T(std::forward<Args>(args)...));
    }
    T &operator*() const & {
        return *data;
    }
    T *operator->() const & {
        return data;
    }
    operator bool() const {
        return data;
    }
};
template <typename T>
class Box {
    T *data;
    Box(T *data):
        data(data) {}
public:
    Box(Box const &other):
        data(new T(*other.data)) {}
    Box(Box &&other):
        data(other.data) {
        other.data = nullptr;
    }
    Box &operator=(Box const &other) {
        if (this != &other) {
            delete data;
            data = new T(*other.data);
        }
        return *this;
    }
    Box &operator=(Box &&other) {
        assert(this != &other); // self-assignment is not allowed
        delete data;
        data = other.data;
        other.data = nullptr;
        return *this;
    }
    ~Box() {
        delete data;
    }
    template <typename... Args>
    static Box make(Args &&...args) {
        return Box(new T(std::forward<Args>(args)...));
    }
    T const &operator*() const & {
        return *data;
    }
    T const *operator->() const & {
        return data;
    }
    T &operator*() & {
        return *data;
    }
    T *operator->() & {
        return data;
    }
};

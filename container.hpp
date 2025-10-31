#pragma once

#include <cassert>
#include <utility>

// The difference between Opt and Box is that Opt can be initialized or
// assigned to nullptr, while Box can only be null after being moved.
// Besides, the data in const Opt can still be modified, while the data
// in const Box cannot be modified.
// Both Opt and Box can be copied and moved. The data in them will also
// be copied when they are copied.

template<typename T>
class Opt {
    T *data;

    Opt(T *data)
        : data(data) {}

public:
    Opt(std::nullptr_t)
        : data(nullptr) {}

    Opt(Opt const &other)
        : data(other.data ? new T(*other.data) : nullptr) {}

    Opt(Opt &&other)
        : data(std::exchange(other.data, nullptr)) {}

    Opt &operator=(Opt other) {
        std::swap(this->data, other.data);
        return *this;
    }

    ~Opt() {
        delete data;
    }

    template<typename... Args>
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

template<typename T>
class Box {
    T *data;

    Box(T *data)
        : data(data) {}

public:
    Box(Box const &other)
        : data(new T(*other.data)) {}

    Box(Box &&other)
        : data(std::exchange(other.data, nullptr)) {}

    Box &operator=(Box other) {
        std::swap(this->data, other.data);
        return *this;
    }

    ~Box() {
        delete data;
    }

    template<typename... Args>
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

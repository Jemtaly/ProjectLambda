#pragma once
#include <utility>
#include <cassert>
// The difference between Nxt and Box is that Nxt can be initialized or
// assigned to nullptr, while Box can only be null after being moved.
// Besides, the data in const Nxt can still be modified, while the data
// in const Box cannot be modified.
// Both Nxt and Box can be copied and moved. The data in them will also
// be copied when they are copied.
template <typename T>
class Nxt {
    T *data;
    Nxt(T *data):
        data(data) {}
public:
    Nxt(std::nullptr_t):
        data(nullptr) {}
    Nxt(Nxt const &other):
        data(other.data ? new T(*other.data) : nullptr) {}
    Nxt(Nxt &&other):
        data(other.data) {
        other.data = nullptr;
    }
    Nxt &operator=(std::nullptr_t) {
        delete data;
        data = nullptr;
        return *this;
    }
    Nxt &operator=(Nxt const &other) {
        if (this != &other) {
            delete data;
            data = other.data ? new T(*other.data) : nullptr;
        }
        return *this;
    }
    Nxt &operator=(Nxt &&other) {
        assert(this != &other); // self-assignment is not allowed
        delete data;
        data = other.data;
        other.data = nullptr;
        return *this;
    }
    ~Nxt() {
        delete data;
    }
    template <typename... Args>
    static Nxt make(Args &&...args) {
        return Nxt(new T(std::forward<Args>(args)...));
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

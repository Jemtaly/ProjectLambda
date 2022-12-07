template <typename T>
class Node {
	T *data;
	Node(T *data):
		data(data) {}
public:
	Node(std::nullptr_t):
		data(nullptr) {}
	Node(Node const &other):
		data(other.data ? new T(*other.data) : nullptr) {}
	Node(Node &&other):
		data(other.data) {
		other.data = nullptr;
	}
	Node &operator=(std::nullptr_t) {
		delete data;
		data = nullptr;
		return *this;
	}
	Node &operator=(Node const &other) {
		if (this != &other) {
			delete data;
			data = other.data ? new T(*other.data) : nullptr;
		}
		return *this;
	}
	Node &operator=(Node &&other) {
		if (this != &other) {
			delete data;
			data = other.data;
			other.data = nullptr;
		}
		return *this;
	}
	~Node() {
		delete data;
	}
	static Node make(T const &value) {
		return Node(new T(value));
	}
	static Node make(T &&value) {
		return Node(new T(std::move(value)));
	}
	template <typename... Args>
	static Node make(Args &&...args) {
		return Node(new T(std::forward<Args>(args)...));
	}
	T &operator*() const {
		return *data;
	}
	T *operator->() const {
		return data;
	}
	operator bool() const {
		return data;
	}
};

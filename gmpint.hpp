#pragma once
#include <gmpxx.h>
class StrInt {
	mpz_class data;
	StrInt(std::string const &str):
		data(str) {}
	template <typename T>
	StrInt(__gmp_expr<mpz_t, T> const &expr):
		data(expr) {}
public:
	static StrInt from_string(std::string const &str) {
		return StrInt(str);
	}
	std::string to_string() const {
		return data.get_str();
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

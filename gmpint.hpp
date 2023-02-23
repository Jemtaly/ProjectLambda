#pragma once
#include <gmpxx.h>
class StrInt {
	mpz_class data;
public:
	StrInt(std::string const &str):
		data(str) {}
	StrInt(mpz_class const &data):
		data(data) {}
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

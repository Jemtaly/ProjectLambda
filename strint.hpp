#pragma once
#include <stdint.h>
#include <string>
class StrInt {
	size_t len;
	int8_t *abs;
	size_t *ctr;
	auto const &get(size_t const &i) const {
		return abs[i < len ? i : len];
	}
	StrInt(size_t const &rlen, int8_t *const &rabs):
		len(rlen), abs(rabs), ctr(new size_t(1)) {
		while (len && abs[len - 1] == abs[len]) {
			len--;
		}
	}
public:
	StrInt(StrInt const &rval):
		len(rval.len), abs(rval.abs), ctr(rval.ctr) {
		++*ctr;
	}
	StrInt &operator=(StrInt const &rval) {
		++*rval.ctr;
		if (--*ctr == 0) {
			delete[] abs;
			delete ctr;
		}
		len = rval.len;
		abs = rval.abs;
		ctr = rval.ctr;
		return *this;
	}
	~StrInt() {
		if (--*ctr == 0) {
			delete[] abs;
			delete ctr;
		}
	}
	static StrInt from_string(std::string const &str) {
		size_t len = str.size() - (str[0] == '+' || str[0] == '-');
		int8_t *abs = new int8_t[len + 1];
		if (str[0] == '-') {
			int8_t d = 10;
			for (size_t i = str.size() - 1, j = 0; j < len; i--, j++) {
				if (str[i] < '0' || str[i] > '9') {
					delete[] abs;
					throw std::exception();
				} else {
					d = '9' - str[i] + (d == 10);
					abs[j] = d == 10 ? 0 : d;
				}
			}
			abs[len] = d == 10 ? 0 : 9;
		} else {
			for (size_t i = str.size() - 1, j = 0; j < len; i--, j++) {
				if (str[i] < '0' || str[i] > '9') {
					delete[] abs;
					throw std::exception();
				} else {
					abs[j] = str[i] - '0';
				}
			}
			abs[len] = 0;
		}
		return StrInt(len, abs);
	}
	std::string to_string() const {
		if (abs[len]) {
			char str[len + 3];
			str[len + 2] = 0;
			int8_t d = 10;
			for (size_t i = 0, j = len + 1; i < len; i++, j--) {
				d = 9 - abs[i] + (d == 10);
				str[j] = d == 10 ? '0' : '0' + d;
			}
			if (d == 10) {
				str[1] = '1';
				str[0] = '-';
				return str + 0;
			} else {
				str[1] = '-';
				return str + 1;
			}
		} else if (len) {
			char str[len + 1];
			str[len] = 0;
			for (size_t i = 0, j = len - 1; i < len; i++, j--) {
				str[j] = abs[i] + '0';
			}
			return str;
		} else {
			return "0";
		}
	}
	friend inline StrInt operator+(StrInt const &, StrInt const &);
	friend inline StrInt operator-(StrInt const &, StrInt const &);
	friend inline StrInt operator*(StrInt const &, StrInt const &);
	template <bool>
	friend inline StrInt divmod(StrInt const &, StrInt const &);
	friend inline StrInt operator/(StrInt const &, StrInt const &);
	friend inline StrInt operator%(StrInt const &, StrInt const &);
	template <bool, bool, bool>
	friend inline bool compare(StrInt const &, StrInt const &);
	friend inline bool operator>(StrInt const &, StrInt const &);
	friend inline bool operator<(StrInt const &, StrInt const &);
	friend inline bool operator>=(StrInt const &, StrInt const &);
	friend inline bool operator<=(StrInt const &, StrInt const &);
	friend inline bool operator==(StrInt const &, StrInt const &);
	friend inline bool operator!=(StrInt const &, StrInt const &);
};
StrInt operator+(StrInt const &lhs, StrInt const &rhs) {
	size_t len = (lhs.len > rhs.len ? lhs.len : rhs.len) + 1;
	int8_t *abs = new int8_t[len + 1];
	int8_t s = 0;
	for (size_t i = 0; i <= len; i++) {
		s = lhs.get(i) + rhs.get(i) + (s >= 10);
		abs[i] = s >= 10 ? s - 10 : s;
	}
	return StrInt(len, abs);
}
StrInt operator-(StrInt const &lhs, StrInt const &rhs) {
	size_t len = (lhs.len > rhs.len ? lhs.len : rhs.len) + 1;
	int8_t *abs = new int8_t[len + 1];
	int8_t d = 0;
	for (size_t i = 0; i <= len; i++) {
		d = lhs.get(i) - rhs.get(i) - (d < 0);
		abs[i] = d < 0 ? d + 10 : d;
	}
	return StrInt(len, abs);
}
StrInt operator*(StrInt const &lhs, StrInt const &rhs) {
	size_t len = lhs.len + rhs.len + 1;
	int8_t *abs = new int8_t[len + 1]();
	for (size_t i = 0; i <= len; i++) {
		int8_t p = 0, s = 0;
		for (size_t j = 0; i + j <= len; j++) {
			p = lhs.get(j) * rhs.get(i) + p / 10;
			s = p % 10 + abs[i + j] + (s >= 10);
			abs[i + j] = s >= 10 ? s - 10 : s;
		}
	}
	return StrInt(len, abs);
}
template <bool select>
StrInt divmod(StrInt const &lhs, StrInt const &rhs) {
	size_t len = lhs.len + rhs.len;
	int8_t *pabs = new int8_t[len + 1], *nabs = new int8_t[len + 1];
	int8_t *qabs = new int8_t[lhs.len + 1];
	int8_t *rabs = new int8_t[rhs.len + 1];
	for (size_t i = 0; i <= len; i++) {
		pabs[i] = lhs.get(i);
	}
	if (lhs.abs[lhs.len] == rhs.abs[rhs.len]) {
		for (size_t i = lhs.len; i <= lhs.len; i--) {
			for (qabs[i] = 0;; qabs[i]++) {
				int8_t d = 0;
				for (size_t j = 0; i + j <= len; j++) {
					d = pabs[i + j] - rhs.get(j) - (d < 0);
					nabs[i + j] = d < 0 ? d + 10 : d;
				}
				if (nabs[len] != pabs[len]) {
					break;
				}
				for (size_t j = 0; i + j <= len; j++) {
					pabs[i + j] = nabs[i + j];
				}
			}
		}
		for (size_t i = 0; i <= rhs.len; i++) {
			rabs[i] = pabs[i];
		}
	} else {
		for (size_t i = lhs.len; i <= lhs.len; i--) {
			for (qabs[i] = 9;; qabs[i]--) {
				int8_t s = 0;
				for (size_t j = 0; i + j <= len; j++) {
					s = pabs[i + j] + rhs.get(j) + (s >= 10);
					nabs[i + j] = s >= 10 ? s - 10 : s;
				}
				if (nabs[len] != pabs[len]) {
					break;
				}
				for (size_t j = 0; i + j <= len; j++) {
					pabs[i + j] = nabs[i + j];
				}
			}
		}
		for (size_t i = 0; i <= rhs.len; i++) {
			rabs[i] = nabs[i];
		}
	}
	delete[] pabs;
	delete[] nabs;
	if (select) {
		delete[] qabs;
		return StrInt(rhs.len, rabs);
	} else {
		delete[] rabs;
		return StrInt(lhs.len, qabs);
	}
}
StrInt operator/(StrInt const &lhs, StrInt const &rhs) {
	return divmod<0>(lhs, rhs);
}
StrInt operator%(StrInt const &lhs, StrInt const &rhs) {
	return divmod<1>(lhs, rhs);
}
template <bool gt, bool eq, bool lt>
bool compare(StrInt const &lhs, StrInt const &rhs) {
	if (lhs.abs[lhs.len] < rhs.abs[rhs.len]) {
		return gt;
	}
	if (lhs.abs[lhs.len] > rhs.abs[rhs.len]) {
		return lt;
	}
	for (size_t m = lhs.len > rhs.len ? lhs.len : rhs.len, i = m - 1; i < m; i--) {
		if (lhs.get(i) > rhs.get(i)) {
			return gt;
		}
		if (lhs.get(i) < rhs.get(i)) {
			return lt;
		}
	}
	return eq;
}
bool operator>(StrInt const &lhs, StrInt const &rhs) {
	return compare<1, 0, 0>(lhs, rhs);
}
bool operator<=(StrInt const &lhs, StrInt const &rhs) {
	return compare<0, 1, 1>(lhs, rhs);
}
bool operator<(StrInt const &lhs, StrInt const &rhs) {
	return compare<0, 0, 1>(lhs, rhs);
}
bool operator>=(StrInt const &lhs, StrInt const &rhs) {
	return compare<1, 1, 0>(lhs, rhs);
}
bool operator==(StrInt const &lhs, StrInt const &rhs) {
	return compare<0, 1, 0>(lhs, rhs);
}
bool operator!=(StrInt const &lhs, StrInt const &rhs) {
	return compare<1, 0, 1>(lhs, rhs);
}
inline StrInt &operator+=(StrInt &lhs, StrInt const &rhs) {
	return lhs = lhs + rhs;
}
inline StrInt &operator-=(StrInt &lhs, StrInt const &rhs) {
	return lhs = lhs - rhs;
}
inline StrInt &operator*=(StrInt &lhs, StrInt const &rhs) {
	return lhs = lhs * rhs;
}
inline StrInt &operator/=(StrInt &lhs, StrInt const &rhs) {
	return lhs = lhs / rhs;
}
inline StrInt &operator%=(StrInt &lhs, StrInt const &rhs) {
	return lhs = lhs % rhs;
}

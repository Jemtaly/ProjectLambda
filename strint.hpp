#pragma once
#include <stdint.h>
#include <string>
class StrInt {
	size_t len;
	int8_t *abs;
	StrInt(size_t const &rlen, int8_t *const &rabs):
		len(rlen), abs(rabs) {
		while (len && abs[len - 1] == abs[len]) {
			len--;
		}
	}
	auto const &get(size_t const &i) const {
		return abs[i < len ? i : len];
	}
public:
	StrInt(StrInt &&rval):
		len(rval.len), abs(rval.abs) {
		rval.abs = nullptr;
	}
	~StrInt() {
		delete[] abs;
	}
	StrInt(std::string const &str):
		len(str.size() - (str[0] == '+' || str[0] == '-')), abs(new int8_t[len + 1]) {
		if (str[0] == '-') {
			int8_t d = 10;
			for (size_t i = str.size() - 1, j = 0; j < len; i--, j++) {
				if (str[i] < '0' || str[i] > '9') {
					delete[] abs;
					throw std::exception();
				} else {
					d = '9' - str[i] + (d == 10);
					abs[j] = d % 10;
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
	}
	operator std::string() const {
		if (abs[len]) {
			char str[len + 3];
			str[len + 2] = 0;
			int8_t d = 10;
			for (size_t i = 0, j = len + 1; i < len; i++, j--) {
				d = 9 - abs[i] + (d == 10);
				str[j] = d % 10 + '0';
			}
			if (d == 10) {
				str[1] = '1';
				str[0] = '-';
				return str;
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
	friend inline StrInt divmod(StrInt const &, StrInt const &, bool const &);
	friend inline StrInt operator/(StrInt const &, StrInt const &);
	friend inline StrInt operator%(StrInt const &, StrInt const &);
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
		abs[i] = s % 10;
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
			abs[i + j] = s % 10;
		}
	}
	return StrInt(len, abs);
}
StrInt divmod(StrInt const &lhs, StrInt const &rhs, bool const &select) {
	size_t len = lhs.len + rhs.len;
	int8_t *pabs = new int8_t[len + 1], *tabs = new int8_t[len + 1];
	int8_t *qabs = new int8_t[lhs.len + 1], *rabs = new int8_t[rhs.len + 1];
	for (size_t i = 0; i <= len; i++) {
		pabs[i] = lhs.get(i);
	}
	if (lhs.abs[lhs.len] == rhs.abs[rhs.len]) {
		for (size_t i = 0; i <= lhs.len; i++) {
			qabs[i] = 0;
		}
		for (size_t i = lhs.len; i != -1; i--) {
			for (;;) {
				int8_t d = 0;
				for (size_t j = 0; i + j <= len; j++) {
					d = pabs[i + j] - rhs.get(j) - (d < 0);
					tabs[i + j] = d < 0 ? d + 10 : d;
				}
				if (tabs[len] != pabs[len]) {
					break;
				}
				qabs[i]++;
				for (size_t j = 0; i + j <= len; j++) {
					pabs[i + j] = tabs[i + j];
				}
			}
		}
		for (size_t i = 0; i <= rhs.len; i++) {
			rabs[i] = pabs[i];
		}
	} else {
		for (size_t i = 0; i <= lhs.len; i++) {
			qabs[i] = 9;
		}
		for (size_t i = lhs.len; i != -1; i--) {
			for (;;) {
				int8_t s = 0;
				for (size_t j = 0; i + j <= len; j++) {
					s = pabs[i + j] + rhs.get(j) + (s >= 10);
					tabs[i + j] = s % 10;
				}
				if (tabs[len] != pabs[len]) {
					break;
				}
				qabs[i]--;
				for (size_t j = 0; i + j <= len; j++) {
					pabs[i + j] = tabs[i + j];
				}
			}
		}
		for (size_t i = 0; i <= rhs.len; i++) {
			rabs[i] = tabs[i];
		}
	}
	delete[] pabs;
	delete[] tabs;
	if (select) {
		delete[] qabs;
		return StrInt(rhs.len, rabs);
	} else {
		delete[] rabs;
		return StrInt(lhs.len, qabs);
	}
}
StrInt operator/(StrInt const &lhs, StrInt const &rhs) {
	return divmod(lhs, rhs, 0);
}
StrInt operator%(StrInt const &lhs, StrInt const &rhs) {
	return divmod(lhs, rhs, 1);
}
bool operator>(StrInt const &lhs, StrInt const &rhs) {
	if (lhs.abs[lhs.len] < rhs.abs[rhs.len]) {
		return true;
	}
	if (lhs.abs[lhs.len] > rhs.abs[rhs.len]) {
		return false;
	}
	for (size_t i = (lhs.len > rhs.len ? lhs.len : rhs.len) - 1; i != -1; i--) {
		if (lhs.get(i) > rhs.get(i)) {
			return true;
		}
		if (lhs.get(i) < rhs.get(i)) {
			return false;
		}
	}
	return false;
}
bool operator>=(StrInt const &lhs, StrInt const &rhs) {
	if (lhs.abs[lhs.len] < rhs.abs[rhs.len]) {
		return true;
	}
	if (lhs.abs[lhs.len] > rhs.abs[rhs.len]) {
		return false;
	}
	for (size_t i = (lhs.len > rhs.len ? lhs.len : rhs.len) - 1; i != -1; i--) {
		if (lhs.get(i) > rhs.get(i)) {
			return true;
		}
		if (lhs.get(i) < rhs.get(i)) {
			return false;
		}
	}
	return true;
}
bool operator<(StrInt const &lhs, StrInt const &rhs) {
	if (lhs.abs[lhs.len] > rhs.abs[rhs.len]) {
		return true;
	}
	if (lhs.abs[lhs.len] < rhs.abs[rhs.len]) {
		return false;
	}
	for (size_t i = (lhs.len > rhs.len ? lhs.len : rhs.len) - 1; i != -1; i--) {
		if (lhs.get(i) < rhs.get(i)) {
			return true;
		}
		if (lhs.get(i) > rhs.get(i)) {
			return false;
		}
	}
	return false;
}
bool operator<=(StrInt const &lhs, StrInt const &rhs) {
	if (lhs.abs[lhs.len] > rhs.abs[rhs.len]) {
		return true;
	}
	if (lhs.abs[lhs.len] < rhs.abs[rhs.len]) {
		return false;
	}
	for (size_t i = (lhs.len > rhs.len ? lhs.len : rhs.len) - 1; i != -1; i--) {
		if (lhs.get(i) < rhs.get(i)) {
			return true;
		}
		if (lhs.get(i) > rhs.get(i)) {
			return false;
		}
	}
	return true;
}
bool operator!=(StrInt const &lhs, StrInt const &rhs) {
	for (size_t i = lhs.len > rhs.len ? lhs.len : rhs.len; i != -1; i--) {
		if (lhs.get(i) != rhs.get(i)) {
			return true;
		}
	}
	return false;
}
bool operator==(StrInt const &lhs, StrInt const &rhs) {
	for (size_t i = lhs.len > rhs.len ? lhs.len : rhs.len; i != -1; i--) {
		if (lhs.get(i) != rhs.get(i)) {
			return false;
		}
	}
	return true;
}

#pragma once
#include <string>
class StrInt {
	bool sign;
	std::string abs;
public:
	StrInt() : sign(0) {}
	StrInt(intmax_t const &val) : sign(val < 0) {
		for (auto i = sign ? -val : val; i; i /= 10)
			abs = (char)('0' + i % 10) + abs;
	}
	StrInt(std::string const &str) : sign(str[0] == '-'), abs(str[0] == '+' || str[0] == '-' ? str.substr(1) : str) {
		for (auto const &c : abs)
			if (c < '0' || c > '9')
				abs.clear();
		while (abs[0] == '0')
			abs = abs.substr(1);
		if (abs.empty())
			sign = 0;
	}
	operator std::string() const {
		return sign ? '-' + abs : abs.size() ? abs : "0";
	}
	friend inline StrInt operator+(StrInt const &);
	friend inline StrInt operator-(StrInt const &);
	friend inline StrInt operator+(StrInt const &, StrInt const &);
	friend inline StrInt operator-(StrInt const &, StrInt const &);
	friend inline StrInt operator*(StrInt const &, StrInt const &);
	friend inline auto divmod(StrInt const &, StrInt const &);
	friend inline StrInt operator/(StrInt const &, StrInt const &);
	friend inline StrInt operator%(StrInt const &, StrInt const &);
	friend inline bool operator>(StrInt const &, StrInt const &);
	friend inline bool operator<(StrInt const &, StrInt const &);
	friend inline bool operator>=(StrInt const &, StrInt const &);
	friend inline bool operator<=(StrInt const &, StrInt const &);
	friend inline bool operator==(StrInt const &, StrInt const &);
	friend inline bool operator!=(StrInt const &, StrInt const &);
};
StrInt operator+(StrInt const &val) {
	return val;
}
StrInt operator-(StrInt const &val) {
	StrInt ret = val;
	if (ret.abs.size())
		ret.sign = !ret.sign;
	return ret;
}
StrInt operator+(StrInt const &lval, StrInt const &rval) {
	if (lval < rval)
		return rval + lval;
	if (lval.sign)
		return -(-rval + -lval);
	if (rval.sign)
		return lval - -rval;
	StrInt sum;
	int8_t s = 0;
	for (size_t li = lval.abs.size(), ri = rval.abs.size(); li;) {
		s = lval.abs[--li] - '0' + (ri ? rval.abs[--ri] - '0' : 0) + (s >= 10);
		sum.abs = (char)((s >= 10 ? s - 10 : s) + '0') + sum.abs;
	}
	if (s >= 10)
		sum.abs = '1' + sum.abs;
	return sum;
}
StrInt operator-(StrInt const &lval, StrInt const &rval) {
	if (lval < rval)
		return -(rval - lval);
	if (lval.sign)
		return (-rval) - (-lval);
	if (rval.sign)
		return lval + -rval;
	StrInt difference;
	int8_t d = 0;
	for (size_t li = lval.abs.size(), ri = rval.abs.size(); li;) {
		d = lval.abs[--li] - '0' - (ri ? rval.abs[--ri] - '0' : 0) - (d < 0);
		difference.abs = (char)((d < 0 ? d + 10 : d) + '0') + difference.abs;
	}
	while (difference.abs[0] == '0')
		difference.abs = difference.abs.substr(1);
	return difference;
}
StrInt operator*(StrInt const &lval, StrInt const &rval) {
	StrInt product;
	for (auto const &r : rval.abs) {
		if (product.abs.size())
			product.abs += '0';
		StrInt temp;
		int8_t t = 0;
		for (size_t li = lval.abs.size(); li;) {
			t = (lval.abs[--li] - '0') * (r - '0') + t / 10;
			temp.abs = (char)(t % 10 + '0') + temp.abs;
		}
		if (t / 10)
			temp.abs = (char)(t / 10 + '0') + temp.abs;
		product = product + temp;
	}
	return rval.sign ? -product : product;
}
auto divmod(StrInt const &lval, StrInt const &rval) {
	struct {
		StrInt quotient, remainder;
	} result;
	if (rval.abs.empty()) {
		result.remainder = lval;
		return result;
	}
	if (rval.sign) {
		result = divmod(-lval, -rval);
		result.remainder = -result.remainder;
		return result;
	}
	for (auto const &l : lval.abs) {
		if (result.remainder.abs.size() || l != '0')
			result.remainder.abs += l;
		char c = '0';
		for (StrInt i = result.remainder; (i = i - rval).sign == 0; result.remainder = i, c++)
			;
		if (result.quotient.abs.size() || c != '0')
			result.quotient.abs += c;
	}
	if (lval.sign) {
		if (result.remainder.abs.size()) {
			result.quotient = -(result.quotient + 1);
			result.remainder = rval - result.remainder;
		} else
			result.quotient = -result.quotient;
	}
	return result;
}
StrInt operator/(StrInt const &lval, StrInt const &rval) {
	return divmod(lval, rval).quotient;
}
StrInt operator%(StrInt const &lval, StrInt const &rval) {
	return divmod(lval, rval).remainder;
}
bool operator>(StrInt const &lval, StrInt const &rval) {
	return lval != rval && (lval.sign xor (lval.sign != rval.sign || lval.abs.size() > rval.abs.size() || lval.abs.size() == rval.abs.size() && lval.abs > rval.abs));
}
bool operator<(StrInt const &lval, StrInt const &rval) {
	return lval != rval && (rval.sign xor (lval.sign != rval.sign || lval.abs.size() < rval.abs.size() || lval.abs.size() == rval.abs.size() && lval.abs < rval.abs));
}
bool operator>=(StrInt const &lval, StrInt const &rval) {
	return lval == rval || (lval.sign xor (lval.sign != rval.sign || lval.abs.size() > rval.abs.size() || lval.abs.size() == rval.abs.size() && lval.abs > rval.abs));
}
bool operator<=(StrInt const &lval, StrInt const &rval) {
	return lval == rval || (rval.sign xor (lval.sign != rval.sign || lval.abs.size() < rval.abs.size() || lval.abs.size() == rval.abs.size() && lval.abs < rval.abs));
}
bool operator==(StrInt const &lval, StrInt const &rval) {
	return rval.sign == lval.sign && rval.abs == lval.abs;
}
bool operator!=(StrInt const &lval, StrInt const &rval) {
	return rval.sign != lval.sign || rval.abs != lval.abs;
}

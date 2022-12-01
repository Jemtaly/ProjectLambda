#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "strint.hpp"
#if defined _WIN32
#include <Windows.h>
#elif defined __unix__
#include <unistd.h>
#endif
typedef StrInt (*opr_t)(StrInt const &, StrInt const &);
typedef bool (*cmp_t)(StrInt const &, StrInt const &);
static std::map<char, opr_t> const oprs = {
	{'+', operator+},
	{'-', operator-},
	{'*', operator*},
	{'/', operator/},
	{'%', operator%},
};
static std::map<char, cmp_t> const cmps = {
	{'=', operator==},
	{'>', operator>},
	{'<', operator<},
};
size_t operator>>(std::string &exp, std::string &sym) {
	size_t i = 0;
	while (exp[i] == ' ') {
		i++;
	}
	size_t j = i;
	for (size_t ctr = 0; ctr || i < exp.size() && exp[i] != ' '; i++) {
		switch (exp[i]) {
		case '(':
		case '[':
			ctr++;
			break;
		case ')':
		case ']':
			ctr--;
			break;
		}
	}
	sym = exp.substr(j, i - j);
	exp = exp.substr(i);
	return i - j;
}
class MyExpr;
static std::map<std::string, MyExpr> defs;
static std::map<std::string, MyExpr> sets;
class MyExpr {
	char type;
	bool done;
	std::variant<std::vector<MyExpr>, std::shared_ptr<MyExpr>, char, StrInt, std::pair<char, StrInt>, std::string> data;
	template <typename T>
	MyExpr(char type, T &&data):
		type(type), done(false), data(data) {}
	void apply(std::shared_ptr<MyExpr> const &ptr, char c = 'a') {
		switch (type) {
		case '$':
			if (std::get<char>(data) == c) {
				type = '#';
				data = ptr;
			}
			break;
		case '^':
			std::get<std::vector<MyExpr>>(data)[0].apply(ptr, c + 1);
			break;
		case '|':
			std::get<std::vector<MyExpr>>(data)[0].apply(ptr, c);
			std::get<std::vector<MyExpr>>(data)[1].apply(ptr, c);
			break;
		}
	}
public:
	static MyExpr parse(std::string &exp) {
		std::string sym;
		if (0 == exp >> sym) {
			return MyExpr('s', "NULL");
		}
		auto res = [&]() {
			if (sym.front() == '$' && sym.size() == 2) {
				return MyExpr('$', sym.back());
			} else if (sym.front() == '&') {
				return MyExpr('&', sym.substr(1));
			} else if (sym.front() == '!') {
				return MyExpr('!', sym.substr(1));
			} else if (sym.front() == '(' && sym.back() == ')') {
				return parse(sym = sym.substr(1, sym.size() - 2));
			} else if (sym.front() == '[' && sym.back() == ']') {
				return MyExpr('^', std::vector<MyExpr>{parse(sym = sym.substr(1, sym.size() - 2))});
			} else if (auto const &o = oprs.find(sym[0]); sym.size() == 1 && o != oprs.end()) {
				return MyExpr('o', o->first);
			} else if (auto const &c = cmps.find(sym[0]); sym.size() == 1 && c != cmps.end()) {
				return MyExpr('c', c->first);
			} else {
				try {
					return MyExpr('i', StrInt(sym));
				} catch (...) {
					return MyExpr('s', sym);
				}
			}
		}();
		while (exp >> sym) {
			res = MyExpr('|', std::vector<MyExpr>{res, parse(sym)});
		}
		return res;
	}
	void calculate() {
		switch (type) {
		case '#': {
			auto const &tmp = std::get<std::shared_ptr<MyExpr>>(data);
			if (not tmp->done) {
				tmp->calculate();
			}
			auto mem = *tmp;
			*this = std::move(mem);
		} break;
		case '!': {
			auto const &tmp = sets.find(std::get<std::string>(data));
			if (tmp == sets.end()) {
				type = 's';
				data = "NULL";
			} else {
				*this = tmp->second;
			}
		} break;
		case '&': {
			auto const &tmp = defs.find(std::get<std::string>(data));
			if (tmp == defs.end()) {
				type = 's';
				data = "NULL";
			} else {
				*this = tmp->second;
				calculate();
			}
		} break;
		case '|': {
			auto &l = std::get<std::vector<MyExpr>>(data)[0];
			auto &r = std::get<std::vector<MyExpr>>(data)[1];
			l.calculate();
			if (l.type == '^') {
				auto mem = std::move(std::get<std::vector<MyExpr>>(l.data)[0]);
				mem.apply(std::make_shared<MyExpr>(std::move(r)));
				*this = std::move(mem);
				calculate();
			} else {
				r.calculate();
				if (r.type == 'i') {
					switch (l.type) {
					case 'o':
					case 'c':
						type = l.type - 32;
						data = std::pair<char, StrInt>{std::get<char>(l.data), std::get<StrInt>(r.data)};
						break;
					case 'O':
						type = 'i';
						data = oprs.at(std::get<std::pair<char, StrInt>>(l.data).first)(std::get<StrInt>(r.data), std::get<std::pair<char, StrInt>>(l.data).second);
						break;
					case 'C':
						type = '^';
						data = cmps.at(std::get<std::pair<char, StrInt>>(l.data).first)(std::get<StrInt>(r.data), std::get<std::pair<char, StrInt>>(l.data).second)
							 ? std::vector<MyExpr>{MyExpr('^', std::vector<MyExpr>{MyExpr('$', 'b')})}
							 : std::vector<MyExpr>{MyExpr('^', std::vector<MyExpr>{MyExpr('$', 'a')})};
						break;
					}
				}
			}
		}
		}
		done = true;
	}
	template <bool par = false>
	std::string translate() const {
		switch (type) {
		case 's':
			return std::get<std::string>(data);
		case 'i':
			return std::string(std::get<StrInt>(data));
		case 'o':
			return std::string{std::get<char>(data)};
		case 'c':
			return std::string{std::get<char>(data)};
		case 'O':
			return std::string{std::get<std::pair<char, StrInt>>(data).first, ':'} + std::string(std::get<std::pair<char, StrInt>>(data).second);
		case 'C':
			return std::string{std::get<std::pair<char, StrInt>>(data).first, ':'} + std::string(std::get<std::pair<char, StrInt>>(data).second);
		case '$':
			return std::string{'$', std::get<char>(data)};
		case '&':
			return "&" + std::get<std::string>(data);
		case '!':
			return "!" + std::get<std::string>(data);
		case '^':
			return "[" + std::get<std::vector<MyExpr>>(data)[0].translate() + "]";
		case '|':
			return (par ? "(" : "") + std::get<std::vector<MyExpr>>(data)[0].translate<0>() + " " + std::get<std::vector<MyExpr>>(data)[1].translate<1>() + (par ? ")" : "");
		case '#':
			return std::get<std::shared_ptr<MyExpr>>(data)->translate();
		}
	}
};
int main(int argc, char *argv[]) {
	bool check_stdin = true, check_stdout = true, check_stderr = true;
#if defined _WIN32
	DWORD dwModeTemp;
	check_stdin = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &dwModeTemp);
	check_stdout = GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &dwModeTemp);
	check_stderr = GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &dwModeTemp);
#elif defined __unix__
	check_stdin = isatty(fileno(stdin));
	check_stdout = isatty(fileno(stdout));
	check_stderr = isatty(fileno(stderr));
#endif
	std::string ps_in = check_stderr && check_stdin ? ">> " : "";
	std::string ps_out = check_stderr && check_stdout ? "=> " : "";
	for (bool end = false; !end;) {
		std::string exp, buf;
		std::cerr << ps_in;
		std::getline(std::cin, exp);
		if ((end = std::cin.eof()) && check_stdin && check_stderr) {
			std::cerr << std::endl;
		}
		exp >> buf;
		if (buf == "dir") {
			for (auto const &l : sets) {
				std::cerr << ps_out;
				std::cout << std::left << std::setw(10) << '!' + (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second.translate() << std::endl;
			}
			for (auto const &l : defs) {
				std::cerr << ps_out;
				std::cout << std::left << std::setw(10) << '&' + (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second.translate() << std::endl;
			}
		} else if (buf == "clr") {
			sets.clear();
			defs.clear();
		} else if (buf == "set") {
			if (exp >> buf) {
				auto tmp = MyExpr::parse(exp);
				tmp.calculate();
				sets.insert_or_assign(buf, std::move(tmp));
			}
		} else if (buf == "def") {
			if (exp >> buf) {
				auto tmp = MyExpr::parse(exp);
				defs.insert_or_assign(buf, std::move(tmp));
			}
		} else if (buf == "cal") {
			auto tmp = MyExpr::parse(exp);
			tmp.calculate();
			std::cerr << ps_out;
			std::cout << tmp.translate() << std::endl;
			sets.insert_or_assign("", std::move(tmp));
		} else if (buf == "fmt") {
			auto tmp = MyExpr::parse(exp);
			std::cerr << ps_out;
			std::cout << tmp.translate() << std::endl;
			defs.insert_or_assign("", std::move(tmp));
		}
	}
	return 0;
}

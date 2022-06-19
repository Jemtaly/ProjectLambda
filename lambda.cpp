#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "strint.hpp"
#if defined _WIN32
#include <Windows.h>
#include <io.h>
#elif defined __unix__
#include <unistd.h>
#endif
struct Arg {
	std::string *pstr;
	bool cal;
	bool fmt;
	Arg(std::string const &str):
		cal(false), fmt(false), pstr(new std::string(str)) {}
	Arg(Arg &&r):
		cal(r.cal), fmt(r.fmt), pstr(r.pstr) {
		r.pstr = nullptr;
	}
	~Arg() {
		delete pstr;
	}
};
static std::vector<Arg> args;
static std::map<std::string, std::string> defs;
static std::map<std::string, std::string> sets;
size_t operator>>(std::string &exp, std::string &sym);
void expwrp(std::string &exp);
void symcal(std::string &sym);
void expcal(std::string &exp);
void symfmt(std::string &sym, bool substitute);
void expfmt(std::string &exp, bool substitute);
int main(int argc, char *argv[]) {
	bool check_stdin = true, check_stdout = true, check_stderr = true;
#if defined _WIN32
	DWORD dwsubstituteTemp;
	check_stdin = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &dwsubstituteTemp);
	check_stdout = GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &dwsubstituteTemp);
	check_stderr = GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &dwsubstituteTemp);
#elif defined __unix__
	check_stdin = isatty(fileno(stdin));
	check_stdout = isatty(fileno(stdout));
	check_stderr = isatty(fileno(stderr));
#endif
	std::string ps_in, ps_out;
	if (check_stdin && check_stderr) {
		ps_in = ">> ";
	}
	if (check_stdout && check_stderr) {
		ps_out = "=> ";
	}
	for (bool end = false; !end;) {
		std::string exp, buf;
		std::cerr << ps_in;
		std::getline(std::cin, exp);
		if ((end = std::cin.eof()) && check_stdin && check_stderr) {
			std::cerr << std::endl;
		}
		exp >> buf;
		if (buf == "cal") {
			expcal(exp);
			expfmt(exp, true);
			args.clear();
			sets[""] = exp;
			std::cerr << ps_out;
			std::cout << exp << std::endl;
		} else if (buf == "fmt") {
			expfmt(exp, false);
			defs[""] = exp;
			std::cerr << ps_out;
			std::cout << exp << std::endl;
		} else if (buf == "set") {
			if (exp >> buf) {
				expcal(exp);
				expfmt(exp, true);
				args.clear();
				sets[buf] = exp;
			}
		} else if (buf == "def") {
			if (exp >> buf) {
				expfmt(exp, false);
				defs[buf] = exp;
			}
		} else if (buf == "clr") {
			sets.clear();
			defs.clear();
		} else if (buf == "dir") {
			for (auto const &l : sets) {
				std::cerr << ps_out;
				std::cout << std::left << std::setw(10) << '!' + (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second << std::endl;
			}
			for (auto const &l : defs) {
				std::cerr << ps_out;
				std::cout << std::left << std::setw(10) << '&' + (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second << std::endl;
			}
		}
	}
	return 0;
}
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
void expwrp(std::string &exp) {
	size_t i = 0;
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
	if (i < exp.size()) {
		exp = '(' + exp + ')';
	}
}
void symcal(std::string &sym) {
	if (sym.front() == '(') {
		sym = sym.substr(1, sym.size() - 2);
		expcal(sym);
	} else if (sym.front() == '#') {
		size_t i = std::stoull(sym.substr(1));
		if (!args[i].cal) {
			symcal(*args[i].pstr);
			args[i].cal = true;
		}
		sym = *args[i].pstr;
	} else if (sym.front() == '!') {
		auto const &l = sets.find(sym.substr(1));
		sym = l == sets.end() ? "()" : l->second;
	} else if (sym.front() == '&') {
		if (auto const &l = defs.find(sym.substr(1)); l == defs.end()) {
			sym = "()";
		} else {
			sym = l->second;
			expcal(sym);
		}
	}
}
void expcal(std::string &exp) {
	std::string fun, arg;
	if ((exp >> fun) == 0) {
		exp = "()";
		return;
	}
	symcal(fun);
	while (exp >> arg) {
		if (fun.front() == '[') {
			fun = fun.substr(1, fun.size() - 2);
			std::string tmp;
			for (size_t i = 0, itn = 0; i < fun.size(); i++) {
				if (tmp.size() && tmp.back() == '$' && fun[i] - 'a' == itn) {
					tmp.back() = '#';
					tmp += std::to_string(args.size());
				} else {
					tmp += fun[i];
					switch (fun[i]) {
					case '[':
						itn++;
						break;
					case ']':
						itn--;
						break;
					}
				}
			}
			args.emplace_back(arg);
			fun = tmp;
			expcal(fun);
		} else {
			symcal(arg);
			expwrp(arg);
			try {
				if (fun.size() == 1 && (oprs.find(fun.front()) != oprs.end() || cmps.find(fun.front()) != cmps.end())) {
					fun += ':' + std::string(StrInt(arg));
				} else if (fun[1] == ':') {
					if (auto const &o = oprs.find(fun.front()); o != oprs.end()) {
						fun = (o->second)(arg, fun.substr(2));
					} else if (auto const &c = cmps.find(fun.front()); c != cmps.end()) {
						fun = (c->second)(arg, fun.substr(2)) ? "[[$b]]" : "[[$a]]";
					} else {
						fun += ' ' + arg;
					}
				} else {
					fun += ' ' + arg;
				}
			} catch (std::string str) {
				fun += ' ' + arg;
			}
		}
	}
	exp = fun;
}
void symfmt(std::string &sym, bool substitute) {
	if (sym.front() == '(') {
		sym = sym.substr(1, sym.size() - 2);
		expfmt(sym, substitute);
	} else if (sym.front() == '[') {
		sym = sym.substr(1, sym.size() - 2);
		expfmt(sym, substitute);
		sym = sym == "()" ? "[]" : '[' + sym + ']';
	} else if (substitute) {
		if (sym.front() == '#') {
			size_t i = std::stoull(sym.substr(1));
			if (!args[i].fmt) {
				(args[i].cal ? expfmt : symfmt)(*args[i].pstr, substitute);
				args[i].fmt = true;
			}
			sym = *args[i].pstr;
		} else if (sym.front() == '!') {
			auto const &l = sets.find(sym.substr(1));
			sym = l == sets.end() ? "()" : l->second;
		} else if (sym.front() == '&') {
			if (auto const &l = defs.find(sym.substr(1)); l == defs.end()) {
				sym = "()";
			} else {
				sym = l->second;
				expfmt(sym, substitute);
			}
		}
	}
}
void expfmt(std::string &exp, bool substitute) {
	std::string fun, arg;
	if ((exp >> fun) == 0) {
		exp = "()";
		return;
	}
	symfmt(fun, substitute);
	while (exp >> arg) {
		symfmt(arg, substitute);
		expwrp(arg);
		fun += ' ' + arg;
	}
	exp = fun;
}

#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "strint.hpp"
#if defined _WIN32
#include <io.h>
#include <windows.h>
#elif defined __unix__
#include <unistd.h>
#endif
typedef StrInt (*opr)(StrInt const &, StrInt const &);
typedef bool (*cmp)(StrInt const &, StrInt const &);
struct Arg {
	std::string exp;
	bool calculated;
	Arg(std::string str) : exp(str), calculated(0) {}
};
static std::map<char, opr> const oprs = {
	{'+', operator+},
	{'-', operator-},
	{'*', operator*},
	{'/', operator/},
	{'%', operator%},
};
static std::map<char, cmp> const cmps = {
	{'=', operator==},
	{'>', operator>},
	{'<', operator<},
};
static std::map<std::string, std::string> const predef = {
	{"TRUE", "[[$b]]"},
	{"FALSE", "[[$a]]"},
	{"NOT", "[$a [[$a]] [[$b]]]"},
	{"AND", "[[$a $b [[$a]]]]"},
	{"OR", "[[$a [[$b]] $b]]"},
	{"XOR", "[[$a $b ($b [[$a]] [[$b]])]]"},
	{"S", "[[[$c $a ($b $a)]]]"},
	{"K", "[[$b]]"},
};
static std::map<std::string, std::string> lambda;
static std::vector<Arg *> args;
size_t operator>>(std::string &str, std::string &buf);
void wordexpr(std::string &argu);
void calcword(std::string &word);
void calcexpr(std::string &expr);
void formword(std::string &word);
void formexpr(std::string &expr);
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
	std::string sym_in, sym_out;
	if (check_stdin && check_stderr)
		sym_in = ">> ";
	if (check_stdout && check_stderr)
		sym_out = "=> ";
	for (bool end = false; !end;) {
		std::string line, buffer;
		std::cerr << sym_in;
		std::getline(std::cin, line);
		if ((end = std::cin.eof()) && check_stdin && check_stderr)
			std::cerr << std::endl;
		line >> buffer;
		if (buffer == "calc") {
			calcexpr(line);
			formexpr(line);
			for (size_t i = 0; i < args.size(); i++)
				delete args[i];
			args.clear();
			std::cerr << sym_out;
			std::cout << line << std::endl;
		} else if (buffer == "set") {
			if (line >> buffer) {
				calcexpr(line);
				formexpr(line);
				lambda[buffer] = line;
			}
		} else if (buffer == "def") {
			if (line >> buffer) {
				formexpr(line);
				lambda[buffer] = line;
			}
		} else if (buffer == "del")
			while (line >> buffer)
				lambda.erase(buffer);
		else if (buffer == "list")
			for (auto const &l : lambda) {
				std::cerr << sym_out;
				std::cout << std::left << std::setw(9) << (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second << std::endl;
			}
	}
	return 0;
}
size_t operator>>(std::string &str, std::string &buf) {
	size_t i = 0;
	for (; str[i] == ' '; i++)
		;
	size_t j = i;
	for (size_t br = 0; br || i < str.size() && str[i] != ' '; i++)
		switch (str[i]) {
		case '(':
		case '[':
			br++;
			break;
		case ')':
		case ']':
			br--;
			break;
		}
	size_t d = i - j;
	buf = str.substr(j, d);
	str = str.substr(i);
	return d;
}
void wordexpr(std::string &argu) {
	size_t i = 0;
	for (size_t br = 0; br || i < argu.size() && argu[i] != ' '; i++)
		switch (argu[i]) {
		case '(':
		case '[':
			br++;
			break;
		case ')':
		case ']':
			br--;
			break;
		}
	if (i < argu.size())
		argu = '(' + argu + ')';
}
void calcword(std::string &word) {
	if (word.front() == '(' and word.back() == ')') {
		word = word.substr(1, word.size() - 2);
		calcexpr(word);
	} else if (word.front() == '#') {
		size_t i;
		std::stringstream(word.substr(1)) >> i;
		if (not args[i]->calculated) {
			calcword(args[i]->exp);
			args[i]->calculated = 1;
		}
		word = args[i]->exp;
	} else if (word.front() == '&') {
		if (auto const &l = lambda.find(word.substr(1)); l != lambda.end()) {
			word = l->second;
			calcexpr(word);
		}
	} else if (auto const &p = predef.find(word); p != predef.end())
		word = p->second;
}
void calcexpr(std::string &expr) {
	std::string func, argu;
	if ((expr >> func) == 0) {
		expr = "()";
		return;
	}
	calcword(func);
	while (expr >> argu)
		if (func.front() == '[' and func.back() == ']') {
			func = func.substr(1, func.size() - 2);
			std::string temp;
			for (size_t i = 0, nest = 0; i < func.size(); i++)
				if (temp.size() && temp.back() == '$' && func[i] - 'a' == nest) {
					temp.back() = '#';
					temp += std::to_string(args.size());
				} else {
					temp += func[i];
					switch (func[i]) {
					case '[':
						nest++;
						break;
					case ']':
						nest--;
						break;
					}
				}
			args.push_back(new Arg(argu));
			func = temp;
			calcexpr(func);
		} else {
			calcword(argu);
			wordexpr(argu);
			try {
				if (func.size() == 1 && (oprs.find(func.front()) != oprs.end() || cmps.find(func.front()) != cmps.end()))
					func += ':' + std::string(StrInt(argu));
				else if (func[1] == ':')
					if (auto const &o = oprs.find(func.front()); o != oprs.end())
						func = (o->second)(argu, func.substr(2));
					else if (auto const &c = cmps.find(func.front()); c != cmps.end())
						func = (c->second)(argu, func.substr(2)) ? "[[$b]]" : "[[$a]]";
					else
						func += ' ' + argu;
				else
					func += ' ' + argu;
			} catch (std::string str) {
				func += ' ' + argu;
			}
		}
	expr = func;
}
void formword(std::string &word) {
	if (word.front() == '#') {
		size_t i;
		std::stringstream(word.substr(1)) >> i;
		word = args[i]->exp;
		formword(word);
	} else if (word.front() == '(' and word.back() == ')') {
		word = word.substr(1, word.size() - 2);
		formexpr(word);
	} else if (word.front() == '[' and word.back() == ']') {
		word = word.substr(1, word.size() - 2);
		formexpr(word);
		word = '[' + word + ']';
	}
}
void formexpr(std::string &expr) {
	std::string func, argu;
	if ((expr >> func) == 0) {
		expr = "()";
		return;
	}
	formword(func);
	while (expr >> argu) {
		formword(argu);
		wordexpr(argu);
		func += ' ' + argu;
	}
	expr = func;
}

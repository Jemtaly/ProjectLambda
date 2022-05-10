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
struct Argument {
	std::string exp;
	bool calculated;
	Argument(std::string str) : exp(str), calculated(0) {}
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
static std::vector<Argument *> args;
size_t operator>>(std::string &str, std::string &buffer) {
	size_t i = 0;
	for (; str[i] == ' '; i++)
		;
	size_t j = i;
	for (size_t br = 0, size = str.size(); br || i < size && str[i] != ' '; i++)
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
	buffer = str.substr(j, d);
	str = str.substr(i);
	return d;
}
std::string &operator<<(std::string &expression, bool const &calc) {
	std::string function, argument;
	if (expression >> function)
		switch (function[0]) {
		case '(':
			function = function.substr(1, function.size() - 2);
			function << calc;
			break;
		case '$': {
			size_t i;
			std::stringstream(function.substr(1)) >> i;
			if (not args[i]->calculated) {
				args[i]->exp << calc;
				args[i]->calculated = 1;
			}
			function = args[i]->exp;
			break;
		}
		case '&':
			if (calc)
				if (auto const &l = lambda.find(function.substr(1)); l != lambda.end()) {
					function = l->second;
					function << calc;
				}
			break;
		default:
			if (auto const &p = predef.find(function); p != predef.end())
				function = p->second;
			break;
		}
	else
		return expression = "()";
	while (expression >> argument)
		if (calc && function[0] == '[') {
			function = function.substr(1, function.size() - 2);
			std::string temp;
			for (size_t nest = 0, i = 0, size = function.size(); i < size; i++)
				if (i && function[i - 1] == '$' && function[i] - 'a' == nest)
					temp += std::to_string(args.size());
				else {
					temp += function[i];
					switch (function[i]) {
					case '[':
						nest++;
						break;
					case ']':
						nest--;
						break;
					}
				}
			args.push_back(new Argument(argument));
			function = temp;
			function << calc;
		} else {
			argument << calc;
			for (size_t br = 0, i = 0, size = argument.size(); br || i < size && (argument[i] == ' ' ? argument = '(' + argument + ')', 0 : 1); i++)
				switch (argument[i]) {
				case '(':
				case '[':
					br++;
					break;
				case ')':
				case ']':
					br--;
					break;
				}
			try {
				if (function.size() == 1 && (oprs.find(function[0]) != oprs.end() || cmps.find(function[0]) != cmps.end()))
					function += ':' + std::string(StrInt(argument));
				else if (function[1] == ':')
					if (auto const &o = oprs.find(function[0]); o != oprs.end())
						function = (o->second)(argument, function.substr(2));
					else if (auto const &c = cmps.find(function[0]); c != cmps.end())
						function = (c->second)(argument, function.substr(2)) ? "[[$b]]" : "[[$a]]";
					else
						function += ' ' + argument;
				else
					function += ' ' + argument;
			} catch (std::string str) {
				function += ' ' + argument;
			}
		}
	return expression = function;
}
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
	for (bool eof = false; !eof;) {
		std::string line, buffer;
		std::cerr << sym_in;
		std::getline(std::cin, line);
		if ((eof = std::cin.eof()) && check_stdin && check_stderr)
			std::cerr << std::endl;
		line >> buffer;
		if (buffer == "exit")
			break;
		else if (buffer == "list")
			for (auto const &l : lambda) {
				std::cerr << sym_out;
				std::cout << std::left << std::setw(9) << (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second << std::endl;
			}
		else if (buffer == "del")
			while (line >> buffer)
				lambda.erase(buffer);
		else if (buffer == "def")
			if (line >> buffer) {
				line << false;
				lambda[buffer] = line;
			} else;
		else if (buffer == "calc") {
			line << true;
			for (size_t i = 0; i < args.size(); delete args[i++])
				;
			args.clear();
			std::cerr << sym_out;
			std::cout << line << std::endl;
		}
	}
}

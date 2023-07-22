#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include "node.hpp"
#include "strint.hpp"
#if defined _WIN32
#include <Windows.h>
#elif defined __unix__
#include <unistd.h>
#endif
#define SET 1
#define DEF 0
typedef StrInt (*opr_t)(StrInt const &, StrInt const &);
typedef bool (*cmp_t)(StrInt const &, StrInt const &);
inline std::unordered_map<char, opr_t> const oprs = {
    {'+', operator+},
    {'-', operator-},
    {'*', operator*},
    {'/', operator/},
    {'%', operator%},
};
inline std::unordered_map<char, cmp_t> const cmps = {
    {'>', operator>},
    {'<', operator<},
    {'=', operator==},
};
inline bool operator>>(std::string &exp, std::string &sym) {
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
    return i > j;
}
class Expr {
    template <bool spc>
    static inline std::unordered_map<std::string, Expr const> dict;
    char type;
    std::variant<Node<Expr>, Node<std::pair<Expr, Expr>>, std::shared_ptr<std::pair<Expr, bool>>, std::pair<char, StrInt>, char, StrInt, std::string> data;
    friend Node<Expr>;
    template <typename T>
    Expr(char type, T &&data):
        type(type), data(static_cast<T &&>(data)) {}
    void apply(std::shared_ptr<std::pair<Expr, bool>> const &ptr, char c = 'a') {
        switch (type) {
        case '$':
            if (std::get<char>(data) == c) {
                type = '#';
                data = ptr;
            }
            break;
        case '^':
            std::get<Node<Expr>>(data)->apply(ptr, c + 1);
            break;
        case '|':
            std::get<Node<std::pair<Expr, Expr>>>(data)->first.apply(ptr, c);
            std::get<Node<std::pair<Expr, Expr>>>(data)->second.apply(ptr, c);
            break;
        }
    }
    template <bool spc>
    void get(std::string const &name) {
        if (auto const &it = dict<spc>.find(name); it != dict<spc>.end()) {
            *this = it->second;
            if constexpr (spc == DEF) {
                calculate();
            }
        } else {
            type = 's';
            data = "NULL";
        }
    }
public:
    template <bool spc, typename T>
    static auto &put(std::string const &name, T &&expr) {
        if constexpr (spc == SET) {
            expr.calculate();
        }
        if (auto const &it = dict<spc>.find(name); it != dict<spc>.end()) {
            dict<spc>.erase(it);
        }
        return dict<spc>.emplace(name, std::forward<T>(expr)).first->second;
    }
    template <bool spc>
    static auto const &dir() {
        return dict<spc>;
    }
    template <bool spc>
    static void clr() {
        dict<spc>.clear();
    }
    static Expr parse(std::string &&exp) {
        std::string sym;
        if (!(exp >> sym)) {
            return Expr('s', "NULL");
        }
        auto res = [&]() {
            if (sym.front() == '$' && sym.size() == 2) {
                return Expr('$', sym.back());
            } else if (sym.front() == '&') {
                return Expr('&', sym.substr(1));
            } else if (sym.front() == '!') {
                return Expr('!', sym.substr(1));
            } else if (sym.front() == '(' && sym.back() == ')') {
                return parse(sym.substr(1, sym.size() - 2));
            } else if (sym.front() == '[' && sym.back() == ']') {
                return Expr('^', Node<Expr>::make(parse(sym.substr(1, sym.size() - 2))));
            } else if (auto const &o = oprs.find(sym[0]); sym.size() == 1 && o != oprs.end()) {
                return Expr('o', o->first);
            } else if (auto const &c = cmps.find(sym[0]); sym.size() == 1 && c != cmps.end()) {
                return Expr('c', c->first);
            } else {
                try {
                    return Expr('i', StrInt::from_string(sym));
                } catch (...) {
                    return Expr('s', sym);
                }
            }
        }();
        while (exp >> sym) {
            res = Expr('|', Node<std::pair<Expr, Expr>>::make(std::move(res), parse(std::move(sym))));
        }
        return res;
    }
    void calculate() {
        switch (type) {
        case '!': {
            get<SET>(std::get<std::string>(data));
        } break;
        case '&': {
            get<DEF>(std::get<std::string>(data));
        } break;
        case '#': {
            auto tmp = std::get<std::shared_ptr<std::pair<Expr, bool>>>(data);
            if (not tmp->second) {
                tmp->first.calculate();
                tmp->second = true;
            }
            *this = tmp->first;
        } break;
        case '|': {
            auto &l = std::get<Node<std::pair<Expr, Expr>>>(data)->first;
            auto &r = std::get<Node<std::pair<Expr, Expr>>>(data)->second;
            l.calculate();
            if (l.type == '^') {
                auto nod = std::move(std::get<Node<Expr>>(l.data));
                nod->apply(std::make_shared<std::pair<Expr, bool>>(std::move(r), false));
                *this = std::move(*nod);
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
                             ? Node<Expr>::make('^', Node<Expr>::make('$', 'b'))
                             : Node<Expr>::make('^', Node<Expr>::make('$', 'a'));
                        break;
                    }
                }
            }
        } break;
        }
    }
    template <bool par = false>
    std::string translate() const {
        switch (type) {
        case 's':
            return std::get<std::string>(data);
        case 'i':
            return std::get<StrInt>(data).to_string();
        case 'o':
        case 'c':
            return std::string{std::get<char>(data)};
        case 'O':
        case 'C':
            return (par ? "(" : "") + std::string{std::get<std::pair<char, StrInt>>(data).first, ' '} + std::get<std::pair<char, StrInt>>(data).second.to_string() + (par ? ")" : "");
        case '|':
            return (par ? "(" : "") + std::get<Node<std::pair<Expr, Expr>>>(data)->first.translate<0>() + " " + std::get<Node<std::pair<Expr, Expr>>>(data)->second.translate<1>() + (par ? ")" : "");
        case '^':
            return "[" + std::get<Node<Expr>>(data)->translate<0>() + "]";
        case '$':
            return std::string{'$', std::get<char>(data)};
        case '&':
            return "&" + std::get<std::string>(data);
        case '!':
            return "!" + std::get<std::string>(data);
        case '#':
            return std::get<std::shared_ptr<std::pair<Expr, bool>>>(data)->first.translate<0>();
        }
    }
};
int main(int argc, char *argv[]) {
    bool check_stdin = true;
    bool check_stdout = true;
    bool check_stderr = true;
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
            for (auto const &l : Expr::dir<SET>()) {
                std::cerr << ps_out;
                std::cout << std::left << std::setw(10) << '!' + (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second.translate() << std::endl;
            }
            for (auto const &l : Expr::dir<DEF>()) {
                std::cerr << ps_out;
                std::cout << std::left << std::setw(10) << '&' + (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second.translate() << std::endl;
            }
        } else if (buf == "clr") {
            Expr::clr<SET>();
            Expr::clr<DEF>();
        } else if (buf == "set") {
            if (exp >> buf) {
                Expr::put<SET>(buf, Expr::parse(std::move(exp)));
            }
        } else if (buf == "def") {
            if (exp >> buf) {
                Expr::put<DEF>(buf, Expr::parse(std::move(exp)));
            }
        } else if (buf == "cal") {
            auto &res = Expr::put<SET>("", Expr::parse(std::move(exp)));
            std::cerr << ps_out;
            std::cout << res.translate() << std::endl;
        } else if (buf == "fmt") {
            auto &res = Expr::put<DEF>("", Expr::parse(std::move(exp)));
            std::cerr << ps_out;
            std::cout << res.translate() << std::endl;
        }
    }
    return 0;
}

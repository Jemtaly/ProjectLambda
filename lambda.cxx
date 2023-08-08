#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include "node.hpp"
#ifdef USE_GMP
#include "gmpint.hpp"
#else
#include "strint.hpp"
#endif
#if defined _WIN32
#include <Windows.h>
#elif defined __unix__
#include <unistd.h>
#endif
#define DEF 0
#define SET 1
#ifndef STACK_MAX
#define STACK_MAX 4294967296 // 4 GiB
#endif
inline int *stack_top;
inline bool stack_err() {
    int dummy;
    return (char *)stack_top - (char *)&dummy >= STACK_MAX / 2;
}
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
    std::size_t i = 0;
    while (exp[i] == ' ') {
        i++;
    }
    std::size_t j = i;
    std::size_t c = 0;
    for (; i < exp.size() && (exp[i] != ' ' || c != 0); i++) {
        switch (exp[i]) {
        case '(':
            c++;
            break;
        case ')':
            c--;
            break;
        }
    }
    if (c != 0) {
        throw std::runtime_error("mismatched parentheses");
    }
    sym = exp.substr(j, i - j);
    exp = exp.substr(i);
    return i > j;
}
class Tree {
    friend Node<std::pair<std::string, Tree>>;
    friend Node<std::pair<Tree, Tree>>;
    enum Token: std::size_t {
        Def, Set,
        Par, Arg,
        App, Fun,
        Opr, OprInt,
        Cmp, CmpInt,
        Int,
    };
    std::variant<
        std::string, std::string,
        std::string, std::shared_ptr<std::pair<Tree, bool>>,
        Node<std::pair<Tree, Tree>>, Node<std::pair<std::string, Tree>>,
        std::pair<char, opr_t>, std::pair<std::pair<char, opr_t>, StrInt>,
        std::pair<char, cmp_t>, std::pair<std::pair<char, cmp_t>, StrInt>,
        StrInt>
        var;
    template <typename... Args>
    Tree(Args &&...args): var(std::forward<Args>(args)...) {}
    template <bool Spc>
    static inline std::unordered_map<std::string, Tree const> dct;
public:
    template <bool Spc>
    static auto const &put(std::string const &key, Tree &&val) {
        if constexpr (Spc == SET) {
            val.calculate();
        }
        return dct<Spc>.erase(key), dct<Spc>.emplace(key, std::move(val)).first->second;
    }
    template <bool Spc>
    static auto const &dir() {
        return dct<Spc>;
    }
    template <bool Spc>
    static void clr() {
        return dct<Spc>.clear();
    }
    static Tree parse(std::string &&exp) {
        std::string sym;
        if (not (exp >> sym)) {
            throw std::runtime_error("empty expression");
        }
        auto build = [&](auto &&snd) {
            return std::move(snd);
        };
        return sym.back() == ':'
            ? build(Node<std::pair<std::string, Tree>>::make(sym.substr(0, sym.size() - 1), parse(std::move(exp))))
            : parse(std::move(exp), build(lex(std::move(sym))));
    }
    static Tree parse(std::string &&exp, Tree &&fst) {
        std::string sym;
        if (not (exp >> sym)) {
            return std::move(fst);
        }
        auto build = [&](auto &&snd) {
            return Node<std::pair<Tree, Tree>>::make(std::move(fst), std::move(snd));
        };
        return sym.back() == ':'
            ? build(Node<std::pair<std::string, Tree>>::make(sym.substr(0, sym.size() - 1), parse(std::move(exp))))
            : parse(std::move(exp), build(lex(std::move(sym))));
    }
    static Tree lex(std::string &&sym) {
        if (sym.front() == '(' && sym.back() == ')') {
            return parse(sym.substr(1, sym.size() - 2));
        } else if (sym.front() == '$') {
            return Tree(std::in_place_index<Token::Par>, sym.substr(1));
        } else if (sym.front() == '&') {
            return Tree(std::in_place_index<Token::Def>, sym.substr(1));
        } else if (sym.front() == '!') {
            return Tree(std::in_place_index<Token::Set>, sym.substr(1));
        } else if (auto const &o = oprs.find(sym[0]); sym.size() == 1 && o != oprs.end()) {
            return Tree(std::in_place_index<Token::Opr>, *o);
        } else if (auto const &c = cmps.find(sym[0]); sym.size() == 1 && c != cmps.end()) {
            return Tree(std::in_place_index<Token::Cmp>, *c);
        } else try {
            return Tree(std::in_place_index<Token::Int>, StrInt::from_string(sym));
        } catch (...) {
            throw std::runtime_error("invalid symbol: " + sym);
        }
    }
    void calculate() {
        if (stack_err()) {
            throw std::runtime_error("recursion too deep");
        }
        switch (var.index()) {
        case Token::App: {
            auto &fst = std::get<Token::App>(var)->first;
            auto &snd = std::get<Token::App>(var)->second;
            if (fst.calculate(), fst.var.index() == Token::Fun) {
                auto arg = std::make_shared<std::pair<Tree, bool>>(std::move(snd), false);
                auto par = std::move(std::get<Token::Fun>(fst.var)->first);
                auto tmp = std::move(std::get<Token::Fun>(fst.var)->second);
                *this = std::move(tmp);
                substitute(arg, par);
                calculate();
            } else if (fst.var.index() == Token::Opr && (snd.calculate(), snd.var.index() == Token::Int) &&
                (std::get<Token::Opr>(fst.var).first != '/' && std::get<Token::Opr>(fst.var).first != '%' || std::get<Token::Int>(snd.var))) {
                var = std::make_pair(std::get<Token::Opr>(fst.var), std::get<Token::Int>(snd.var));
            } else if (fst.var.index() == Token::Cmp && (snd.calculate(), snd.var.index() == Token::Int)) {
                var = std::make_pair(std::get<Token::Cmp>(fst.var), std::get<Token::Int>(snd.var));
            } else if (fst.var.index() == Token::OprInt && (snd.calculate(), snd.var.index() == Token::Int)) {
                var = std::get<Token::OprInt>(fst.var).first.second(std::get<Token::Int>(snd.var), std::get<Token::OprInt>(fst.var).second);
            } else if (fst.var.index() == Token::CmpInt && (snd.calculate(), snd.var.index() == Token::Int)) {
                var = std::get<Token::CmpInt>(fst.var).first.second(std::get<Token::Int>(snd.var), std::get<Token::CmpInt>(fst.var).second)
                    ? Node<std::pair<std::string, Tree>>::make("T", Node<std::pair<std::string, Tree>>::make("F", Tree(std::in_place_index<Token::Par>, "T")))
                    : Node<std::pair<std::string, Tree>>::make("T", Node<std::pair<std::string, Tree>>::make("F", Tree(std::in_place_index<Token::Par>, "F")));
            } else {
                throw std::runtime_error("invalid application: " + fst.translate().first + " on " + snd.translate().first);
            }
        } break;
        case Token::Arg: {
            auto arg = std::get<Token::Arg>(var);
            if (not arg->second) {
                arg->first.calculate();
                arg->second = true;
            }
            *this = arg->first;
        } break;
        case Token::Def:
            if (auto const &it = dct<DEF>.find(std::get<Token::Def>(var)); it != dct<DEF>.end()) {
                *this = it->second;
                calculate();
                break;
            }
            throw std::runtime_error("undefined symbol: &" + std::get<Token::Def>(var));
        case Token::Set:
            if (auto const &it = dct<SET>.find(std::get<Token::Set>(var)); it != dct<SET>.end()) {
                std::unordered_map<std::shared_ptr<std::pair<Tree, bool>>, std::shared_ptr<std::pair<Tree, bool>> const> map;
                *this = it->second.deepcopy(map);
                break;
            }
            throw std::runtime_error("undefined symbol: !" + std::get<Token::Set>(var));
        case Token::Par:
            throw std::runtime_error("unbound parameter: $" + std::get<Token::Par>(var));
        }
    }
    void substitute(std::shared_ptr<std::pair<Tree, bool>> const &arg, std::string const &par) {
        switch (var.index()) {
        case Token::App:
            std::get<Token::App>(var)->first.substitute(arg, par);
            std::get<Token::App>(var)->second.substitute(arg, par);
            break;
        case Token::Fun:
            if (std::get<Token::Fun>(var)->first != par) {
                std::get<Token::Fun>(var)->second.substitute(arg, par);
            }
            break;
        case Token::Par:
            if (std::get<Token::Par>(var) == par) {
                var = arg;
            }
            break;
        }
    }
    Tree deepcopy(std::unordered_map<std::shared_ptr<std::pair<Tree, bool>>, std::shared_ptr<std::pair<Tree, bool>> const> &map) const {
        switch (var.index()) {
        case Token::App:
            return Node<std::pair<Tree, Tree>>::make(
                std::get<Token::App>(var)->first.deepcopy(map),
                std::get<Token::App>(var)->second.deepcopy(map));
        case Token::Fun:
            return Node<std::pair<std::string, Tree>>::make(
                std::get<Token::Fun>(var)->first,
                std::get<Token::Fun>(var)->second.deepcopy(map));
        case Token::Arg:
            if (auto const &it = map.find(std::get<Token::Arg>(var)); it != map.end()) {
                return it->second;
            }
            return map.emplace(std::get<Token::Arg>(var), std::make_shared<std::pair<Tree, bool>>(
                std::get<Token::Arg>(var)->first.deepcopy(map),
                std::get<Token::Arg>(var)->second)).first->second;
        default:
            return *this;
        }
    }
    std::pair<std::string, std::pair<bool, bool>> translate() const {
        switch (var.index()) {
        case Token::Fun:
            return {std::get<Token::Fun>(var)->first + ": " + std::get<Token::Fun>(var)->second.translate().first, {1, 0}};
        case Token::Par:
            return {"$" + std::get<Token::Par>(var), {0, 0}};
        case Token::Def:
            return {"&" + std::get<Token::Def>(var), {0, 0}};
        case Token::Set:
            return {"!" + std::get<Token::Set>(var), {0, 0}};
        case Token::Arg:
            return std::get<Token::Arg>(var)->first.translate();
        case Token::Int:
            return {std::get<Token::Int>(var).to_string(), {0, 0}};
        case Token::Opr:
            return {std::string{std::get<Token::Opr>(var).first}, {0, 0}};
        case Token::Cmp:
            return {std::string{std::get<Token::Cmp>(var).first}, {0, 0}};
        case Token::OprInt:
            return {std::string{std::get<Token::OprInt>(var).first.first, ' '} + std::get<Token::OprInt>(var).second.to_string(), {0, 1}};
        case Token::CmpInt:
            return {std::string{std::get<Token::CmpInt>(var).first.first, ' '} + std::get<Token::CmpInt>(var).second.to_string(), {0, 1}};
        case Token::App: {
            auto fst = std::get<Token::App>(var)->first.translate();
            auto snd = std::get<Token::App>(var)->second.translate();
            return {(fst.second.first ? "(" + fst.first + ")" : fst.first) + " " + (snd.second.second ? "(" + snd.first + ")" : snd.first), {snd.second.first && not snd.second.second, 1}};
        } break;
        }
    }
};
int main(int argc, char *argv[]) {
    stack_top = &argc;
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
    for (bool end = false; not end;) {
        std::string exp, buf;
        std::cerr << ps_in;
        std::getline(std::cin, exp);
        if (std::cin.eof()) {
            end = true;
            if (check_stderr && check_stdin) {
                std::cerr << std::endl;
            }
        }
        try {
            if (not (exp >> buf) || buf == "#") {
                continue;
            } else if (buf == "def") {
                if (not (exp >> buf)) {
                    throw std::runtime_error("missing symbol after def");
                }
                Tree::put<DEF>(buf, Tree::parse(std::move(exp)));
            } else if (buf == "set") {
                if (not (exp >> buf)) {
                    throw std::runtime_error("missing symbol after set");
                }
                Tree::put<SET>(buf, Tree::parse(std::move(exp)));
            } else if (buf == "fmt") {
                auto &res = Tree::put<DEF>("", Tree::parse(std::move(exp)));
                std::cerr << ps_out;
                std::cout << res.translate().first << std::endl;
            } else if (buf == "cal") {
                auto &res = Tree::put<SET>("", Tree::parse(std::move(exp)));
                std::cerr << ps_out;
                std::cout << res.translate().first << std::endl;
            } else if (buf == "dir") {
                for (auto const &l : Tree::dir<DEF>()) {
                    std::cerr << ps_out;
                    std::cout << std::left << std::setw(10) << "&" + (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second.translate().first << std::endl;
                }
                for (auto const &l : Tree::dir<SET>()) {
                    std::cerr << ps_out;
                    std::cout << std::left << std::setw(10) << "!" + (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second.translate().first << std::endl;
                }
            } else if (buf == "clr") {
                Tree::clr<DEF>();
                Tree::clr<SET>();
            } else if (buf == "end") {
                end = true;
            } else {
                throw std::runtime_error("unknown command: " + buf);
            }
        } catch (std::exception const &e) {
            std::cerr << "Runtime Error: " << e.what() << std::endl;
        }
    }
    return 0;
}

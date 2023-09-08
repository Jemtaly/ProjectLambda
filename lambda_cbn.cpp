#include <iomanip>
#include <iostream>
#include <optional>
#include <variant>
#include <cassert>
#include <string>
#include <memory>
#include <unordered_map>
#include "container.hpp"
#include "slice.hpp"
#ifdef USE_GMP
#include "gmpint.hpp"
#else
#include "strint.hpp"
#endif
#if defined _WIN32
#include <Windows.h>
#elif defined __unix__
#include <unistd.h>
#include <sys/resource.h>
#endif
#ifndef STACK_SIZE
#define STACK_SIZE 8388608 // 8 MiB
#endif
std::string ps_in;
std::string ps_res;
std::string ps_out;
int *stack_top;
bool stack_err() {
    int dummy;
    return (char *)stack_top - (char *)&dummy >= STACK_SIZE / 2;
}
auto read(Slice &exp) {
    auto i = exp.get_beg();
    auto n = exp.get_end();
    for (;; i++) {
        if (i == n) {
            return exp.reset_to(i, n), exp.from_to(i, i);
        } else if (*i != ' ') {
            break;
        }
    }
    auto j = i;
    auto c = 0;
    for (;; i++) {
        if ((i == n || *i == ' ') && c == 0) {
            return exp.reset_to(i, n), exp.from_to(j, i);
        } else if (i == n) {
            throw std::runtime_error("mismatched parentheses");
        } else if (*i == '(') {
            c++;
        } else if (*i == ')') {
            c--;
        }
    }
}
StrInt operator+(StrInt const &lval, StrInt const &rval);
StrInt operator-(StrInt const &lval, StrInt const &rval);
StrInt operator*(StrInt const &lval, StrInt const &rval);
StrInt operator/(StrInt const &lval, StrInt const &rval);
StrInt operator%(StrInt const &lval, StrInt const &rval);
bool operator>(StrInt const &lval, StrInt const &rval);
bool operator<(StrInt const &lval, StrInt const &rval);
bool operator>=(StrInt const &lval, StrInt const &rval);
bool operator<=(StrInt const &lval, StrInt const &rval);
bool operator==(StrInt const &lval, StrInt const &rval);
bool operator!=(StrInt const &lval, StrInt const &rval);
typedef StrInt (*opr_t)(StrInt const &, StrInt const &);
typedef bool (*cmp_t)(StrInt const &, StrInt const &);
static inline std::unordered_map<char, opr_t> const oprs = {
    {'+', operator+},
    {'-', operator-},
    {'*', operator*},
    {'/', operator/},
    {'%', operator%},
};
static inline std::unordered_map<char, cmp_t> const cmps = {
    {'>', operator>},
    {'<', operator<},
    {'=', operator==},
};
class Tree {
    enum Token: std::size_t {
        Ini, Ell, Int,
        Opr, OprInt,
        Cmp, CmpInt,
        Fun, Out,
        App, Arg, Par,
    };
    using Var = std::variant<
        std::nullopt_t, std::monostate, StrInt,
        std::pair<char, opr_t>, std::pair<std::pair<char, opr_t>, StrInt>,
        std::pair<char, cmp_t>, std::pair<std::pair<char, cmp_t>, StrInt>,
        Box<std::pair<std::string, Tree>>, Box<std::pair<std::string, Tree>>,
        Box<std::pair<Tree, Tree>>, std::shared_ptr<std::pair<Tree, bool>>, std::string>;
    Var var;
    template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<Var, Args &&...>>>
    Tree(Args &&...args): var(std::forward<Args>(args)...) {}
    friend Box<std::pair<std::string, Tree>>;
    friend Box<std::pair<Tree, Tree>>;
    static Tree first(Tree &&fst) {
        if (fst.var.index() == Token::Ini) {
            throw std::runtime_error("empty expression");
        } else {
            return std::move(fst);
        }
    }
    static Tree build(Tree &&fst, Tree &&snd) {
        if (fst.var.index() == Token::Ini) {
            return std::move(snd);
        } else {
            return Box<std::pair<Tree, Tree>>::make(std::move(fst), std::move(snd));
        }
    }
    // static Tree parse(Slice &&exp, Tree &&fst = std::nullopt) {
    //     if (auto sym = read(exp); sym.empty()) {
    //         return first(std::move(fst));
    //     } else if (sym[0] == '\\') {
    //         return build(std::move(fst), Tree(std::in_place_index<Token::Fun>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp)))));
    //     } else if (sym[0] == '|') {
    //         return parse(std::move(exp), Tree(std::in_place_index<Token::Fun>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), first(std::move(fst)))));
    //     } else if (sym[0] == '^') {
    //         return build(std::move(fst), Tree(std::in_place_index<Token::Out>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp)))));
    //     } else if (sym[0] == '@') {
    //         return parse(std::move(exp), Tree(std::in_place_index<Token::Out>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), first(std::move(fst)))));
    //     } else {
    //         return parse(std::move(exp), build(std::move(fst), lex(std::move(sym))));
    //     }
    // }
    static Tree parse(Slice &&exp, Tree &&fun = std::nullopt, Tree &&fst = std::nullopt) {
        if (auto sym = read(exp); sym.empty()) {
            return build(std::move(fun), first(std::move(fst)));
        } else if (sym[0] == '\\') {
            return build(std::move(fun), build(std::move(fst), Tree(std::in_place_index<Token::Fun>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp))))));
        } else if (sym[0] == '|') {
            return parse(std::move(exp), Tree(std::in_place_index<Token::Fun>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), build(std::move(fun), first(std::move(fst))))));
        } else if (sym[0] == '^') {
            return build(std::move(fun), build(std::move(fst), Tree(std::in_place_index<Token::Out>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp))))));
        } else if (sym[0] == '@') {
            return parse(std::move(exp), Tree(std::in_place_index<Token::Out>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), build(std::move(fun), first(std::move(fst))))));
        } else {
            return parse(std::move(exp), std::move(fun), build(std::move(fst), lex(std::move(sym))));
        }
    }
    static Tree lex(Slice const &sym) {
        if (sym[0] == '(' && sym[-1] == ')') {
            return parse(sym(1, -1));
        } else if (sym[0] == '$') {
            return Tree(std::in_place_index<Token::Par>, sym(1, 0));
        } else if (sym.size() == 3 && sym == "...") {
            return Tree(std::in_place_index<Token::Ell>);
        } else if (auto const &o = oprs.find(sym[0]); sym.size() == 1 && o != oprs.end()) {
            return Tree(std::in_place_index<Token::Opr>, *o);
        } else if (auto const &c = cmps.find(sym[0]); sym.size() == 1 && c != cmps.end()) {
            return Tree(std::in_place_index<Token::Cmp>, *c);
        } else try {
            return Tree(std::in_place_index<Token::Int>, StrInt::from_string(sym));
        } catch (...) {
            throw std::runtime_error("invalid symbol: " + std::string(sym));
        }
    }
    void calc() {
        if (stack_err()) {
            throw std::runtime_error("recursion too deep");
        }
        if (auto ttr = std::get_if<Token::App>(&var); ttr) {
            auto &[fst, snd] = **ttr;
            fst.calc();
            if (auto ptr = std::get_if<Token::Ell>(&fst.var); ptr) {
                var.emplace<Token::Ell>();
            } else if (auto ptr = std::get_if<Token::Fun>(&fst.var); ptr) {
                auto &[par, tmp] = **ptr;
                tmp.substitute(std::make_shared<std::pair<Tree, bool>>(std::move(snd), 0), std::move(par));
                *this = Tree(std::move(tmp));
                calc();
            } else if (auto ptr = std::get_if<Token::Out>(&fst.var); ptr) {
                snd.calc();
                auto &[par, tmp] = **ptr;
                tmp.substitute(std::make_shared<std::pair<Tree, bool>>(std::move(snd), 1), std::move(par));
                *this = Tree(std::move(tmp));
                calc();
            } else if (auto ptr = std::get_if<Token::Opr>(&fst.var); ptr) {
                snd.calc();
                if (auto qtr = std::get_if<Token::Int>(&snd.var); qtr && (*qtr || ptr->first != '/' && ptr->first != '%')) {
                    var = std::make_pair(std::move(*ptr), std::move(*qtr));
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto ptr = std::get_if<Token::Cmp>(&fst.var); ptr) {
                snd.calc();
                if (auto qtr = std::get_if<Token::Int>(&snd.var); qtr) {
                    var = std::make_pair(std::move(*ptr), std::move(*qtr));
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto ptr = std::get_if<Token::OprInt>(&fst.var); ptr) {
                snd.calc();
                if (auto qtr = std::get_if<Token::Int>(&snd.var); qtr) {
                    var = ptr->first.second(std::move(*qtr), std::move(ptr->second));
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto ptr = std::get_if<Token::CmpInt>(&fst.var); ptr) {
                snd.calc();
                if (auto qtr = std::get_if<Token::Int>(&snd.var); qtr) {
                    static const auto T = Tree(std::in_place_index<Token::Fun>, Box<std::pair<std::string, Tree>>::make("T", Tree(std::in_place_index<Token::Fun>, Box<std::pair<std::string, Tree>>::make("F", Tree(std::in_place_index<Token::Par>, "T")))));
                    static const auto F = Tree(std::in_place_index<Token::Fun>, Box<std::pair<std::string, Tree>>::make("T", Tree(std::in_place_index<Token::Fun>, Box<std::pair<std::string, Tree>>::make("F", Tree(std::in_place_index<Token::Par>, "F")))));
                    *this = ptr->first.second(std::move(*qtr), std::move(ptr->second)) ? T : F;
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else {
                throw std::runtime_error("invalid function: " + fst.translate());
            }
        } else if (auto ttr = std::get_if<Token::Arg>(&var); ttr) {
            auto &[shr, rec] = **ttr;
            if (not rec) {
                shr.calc();
                rec = true;
            }
            if (ttr->use_count() == 1) {
                *this = Tree(std::move(shr));
            } else {
                *this = shr;
            }
        } else if (auto ttr = std::get_if<Token::Par>(&var); ttr) {
            throw std::runtime_error("unbound variable: $" + *ttr);
        }
    }
    void substitute(std::shared_ptr<std::pair<Tree, bool>> const &arg, std::string const &pin) {
        if (auto ttr = std::get_if<Token::App>(&var); ttr) {
            auto &[fst, snd] = **ttr;
            fst.substitute(arg, pin);
            snd.substitute(arg, pin);
        } else if (auto ttr = std::get_if<Token::Fun>(&var); ttr) {
            auto &[par, tmp] = **ttr;
            if (par != pin) {
                tmp.substitute(arg, pin);
            }
        } else if (auto ttr = std::get_if<Token::Out>(&var); ttr) {
            auto &[par, tmp] = **ttr;
            if (par != pin) {
                tmp.substitute(arg, pin);
            }
        } else if (auto ttr = std::get_if<Token::Par>(&var); ttr) {
            if (*ttr == pin) {
                var.emplace<Token::Arg>(arg);
            }
        }
    }
    static inline std::unordered_map<std::string, std::shared_ptr<std::pair<Tree, bool>>> map;
public:
    static auto const &put(Slice &&exp, std::string const &key, bool calc) {
        auto res = parse(std::move(exp));
        for (auto &[par, tmp] : map) {
            res.substitute(tmp, par);
        }
        map.erase(key);
        if (calc) {
            res.calc();
        }
        return map.emplace(key, std::make_shared<std::pair<Tree, bool>>(std::move(res), calc)).first->second->first;
    }
    static auto const &dir() {
        return map;
    }
    static void clr() {
        return map.clear();
    }
    std::string translate(bool lb = 0, bool rb = 0) const {
        if (auto ttr = std::get_if<Token::Ell>(&var); ttr) {
            return "...";
        } else if (auto ttr = std::get_if<Token::Fun>(&var); ttr) {
            auto &[par, tmp] = **ttr;
            auto s = "\\" + par + " " + tmp.translate(0, rb && !rb);
            return rb ? "(" + s + ")" : s;
        } else if (auto ttr = std::get_if<Token::Out>(&var); ttr) {
            auto &[par, tmp] = **ttr;
            auto s = "^" + par + " " + tmp.translate(0, rb && !rb);
            return rb ? "(" + s + ")" : s;
        } else if (auto ttr = std::get_if<Token::Int>(&var); ttr) {
            return ttr->to_string();
        } else if (auto ttr = std::get_if<Token::Opr>(&var); ttr) {
            return std::string{ttr->first};
        } else if (auto ttr = std::get_if<Token::Cmp>(&var); ttr) {
            return std::string{ttr->first};
        } else if (auto ttr = std::get_if<Token::OprInt>(&var)) {
            auto s = std::string{ttr->first.first, ' '} + ttr->second.to_string();
            return lb ? "(" + s + ")" : s;
        } else if (auto ttr = std::get_if<Token::CmpInt>(&var)) {
            auto s = std::string{ttr->first.first, ' '} + ttr->second.to_string();
            return lb ? "(" + s + ")" : s;
        } else if (auto ttr = std::get_if<Token::App>(&var); ttr) {
            auto &[fst, snd] = **ttr;
            auto s = fst.translate(lb && !lb, 1) + " " + snd.translate(1, rb && !lb);
            return lb ? "(" + s + ")" : s;
        } else if (auto ttr = std::get_if<Token::Arg>(&var); ttr) {
            auto &[shr, rec] = **ttr;
            return shr.translate(lb, rb);
        } else if (auto ttr = std::get_if<Token::Par>(&var); ttr) {
            return "$" + *ttr;
        } else {
            assert(false); // unreachable
        }
    }
};
int main(int argc, char *argv[]) {
    stack_top = &argc;
    bool check_stdin = false;
    bool check_stdout = false;
    bool check_stderr = false;
#if defined _WIN32
    DWORD dwModeTemp;
    check_stdin = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &dwModeTemp);
    check_stdout = GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &dwModeTemp);
    check_stderr = GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &dwModeTemp);
#elif defined __unix__
    check_stdin = isatty(fileno(stdin));
    check_stdout = isatty(fileno(stdout));
    check_stderr = isatty(fileno(stderr));
    struct rlimit rlim;
    getrlimit(RLIMIT_STACK, &rlim);
    rlim.rlim_cur = STACK_SIZE;
    setrlimit(RLIMIT_STACK, &rlim);
#endif
    ps_in = check_stderr && check_stdin ? ">> " : "";
    ps_out = check_stderr && check_stdout ? "=> " : "";
    ps_res = check_stderr && check_stdout ? "== " : "";
    for (bool end = false; not end;) {
        std::cerr << ps_in;
        Slice exp = Slice::getline(std::cin);
        if (std::cin.eof()) {
            end = true;
            if (check_stderr && check_stdin) {
                std::cerr << std::endl;
            }
        }
        try {
            if (auto cmd = read(exp); cmd.empty() || cmd.size() == 1 && cmd == "#") {
                continue;
            } else if (cmd[0] == ':') {
                Tree::put(std::move(exp), cmd(1, 0), 0);
            } else if (cmd.size() == 3 && cmd == "cal") {
                auto &res = Tree::put(std::move(exp), "", 1);
                std::cerr << ps_res;
                std::cout << res.translate() << std::endl;
            } else if (cmd.size() == 3 && cmd == "dir") {
                for (auto &[key, val] : Tree::dir()) {
                    std::cerr << ps_out;
                    std::cout << ":" + key << std::endl;
                }
            } else if (cmd.size() == 3 && cmd == "clr") {
                Tree::clr();
            } else if (cmd.size() == 3 && cmd == "end") {
                end = true;
            } else {
                throw std::runtime_error("unknown command: " + std::string(cmd));
            }
        } catch (std::exception const &e) {
            std::cerr << "Runtime Error: " << e.what() << std::endl;
        }
    }
    return 0;
}

#include <iomanip>
#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <variant>
#include <cassert>
#include "node.hpp"
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
        Ini, Int,
        Opr, OprInt,
        Cmp, CmpInt,
        Fun, Out,
        Par, Arg,
        Def, App,
    };
    using Var = std::variant<
        std::monostate, StrInt,
        std::pair<char, opr_t>, std::pair<std::pair<char, opr_t>, StrInt>,
        std::pair<char, cmp_t>, std::pair<std::pair<char, cmp_t>, StrInt>,
        Node<std::pair<std::string, Tree>>, Node<std::pair<std::string, Tree>>,
        std::string, std::shared_ptr<std::pair<Tree, bool>>,
        std::string, Node<std::pair<Tree, Tree>>>;
    Var var;
    template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<Var, Args &&...>>>
    Tree(Args &&...args): var(std::forward<Args>(args)...) {}
    friend Node<std::pair<std::string, Tree>>;
    friend Node<std::pair<Tree, Tree>>;
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
            return Node<std::pair<Tree, Tree>>::make(std::move(fst), std::move(snd));
        }
    }
    // static Tree parse(Slice &&exp, Tree &&fst = std::monostate()) {
    //     if (auto sym = read(exp); sym.empty()) {
    //         return first(std::move(fst));
    //     } else if (sym[0] == '\\') {
    //         return build(std::move(fst), Tree(std::in_place_index<Token::Fun>, Node<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp)))));
    //     } else if (sym[0] == '|') {
    //         return parse(std::move(exp), Tree(std::in_place_index<Token::Fun>, Node<std::pair<std::string, Tree>>::make(sym(1, 0), first(std::move(fst)))));
    //     } else if (sym[0] == '^') {
    //         return build(std::move(fst), Tree(std::in_place_index<Token::Out>, Node<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp)))));
    //     } else if (sym[0] == '@') {
    //         return parse(std::move(exp), Tree(std::in_place_index<Token::Out>, Node<std::pair<std::string, Tree>>::make(sym(1, 0), first(std::move(fst)))));
    //     } else {
    //         return parse(std::move(exp), build(std::move(fst), lex(std::move(sym))));
    //     }
    // }
    static Tree parse(Slice &&exp, Tree &&fun = std::monostate(), Tree &&fst = std::monostate()) {
        if (auto sym = read(exp); sym.empty()) {
            return build(std::move(fun), first(std::move(fst)));
        } else if (sym[0] == '\\') {
            return build(std::move(fun), build(std::move(fst), Tree(std::in_place_index<Token::Fun>, Node<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp))))));
        } else if (sym[0] == '|') {
            return parse(std::move(exp), Tree(std::in_place_index<Token::Fun>, Node<std::pair<std::string, Tree>>::make(sym(1, 0), build(std::move(fun), first(std::move(fst))))));
        } else if (sym[0] == '^') {
            return build(std::move(fun), build(std::move(fst), Tree(std::in_place_index<Token::Out>, Node<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp))))));
        } else if (sym[0] == '@') {
            return parse(std::move(exp), Tree(std::in_place_index<Token::Out>, Node<std::pair<std::string, Tree>>::make(sym(1, 0), build(std::move(fun), first(std::move(fst))))));
        } else {
            return parse(std::move(exp), std::move(fun), build(std::move(fst), lex(std::move(sym))));
        }
    }
    static Tree lex(Slice const &sym) {
        if (sym[0] == '(' && sym[-1] == ')') {
            return parse(sym(1, -1));
        } else if (sym[0] == '$') {
            return Tree(std::in_place_index<Token::Par>, sym(1, 0));
        } else if (sym[0] == '&') {
            return Tree(std::in_place_index<Token::Def>, sym(1, 0));
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
        switch (var.index()) {
        case Token::App: {
            auto &[fst, snd] = *std::get<Token::App>(var);
            switch (fst.calc(), fst.var.index()) {
            case Token::Fun: {
                auto &[par, tmp] = *std::get<Token::Fun>(fst.var);
                tmp.substitute(std::make_shared<std::pair<Tree, bool>>(std::move(snd), 0), std::move(par));
                *this = Tree(std::move(tmp));
                return calc();
            }
            case Token::Out: {
                snd.calc();
                std::cerr << ps_out;
                std::cout << std::get<0>(snd.translate()) << std::endl;
                auto &[par, tmp] = *std::get<Token::Out>(fst.var);
                tmp.substitute(std::make_shared<std::pair<Tree, bool>>(std::move(snd), 1), std::move(par));
                *this = Tree(std::move(tmp));
                return calc();
            }
            case Token::Opr:
                if (snd.calc(), snd.var.index() == Token::Int && (std::get<Token::Int>(snd.var)) || std::get<Token::Opr>(fst.var).first != '/' && std::get<Token::Opr>(fst.var).first != '%') {
                    var = std::make_pair(std::get<Token::Opr>(fst.var), std::move(std::get<Token::Int>(snd.var)));
                    return;
                }
                throw std::runtime_error("cannot apply " + std::get<0>(fst.translate()) + " on: " + std::get<0>(snd.translate()));
            case Token::Cmp:
                if (snd.calc(), snd.var.index() == Token::Int) {
                    var = std::make_pair(std::get<Token::Cmp>(fst.var), std::move(std::get<Token::Int>(snd.var)));
                    return;
                }
                throw std::runtime_error("cannot apply " + std::get<0>(fst.translate()) + " on: " + std::get<0>(snd.translate()));
            case Token::OprInt:
                if (snd.calc(), snd.var.index() == Token::Int) {
                    var = std::get<Token::OprInt>(fst.var).first.second(std::move(std::get<Token::Int>(snd.var)), std::move(std::get<Token::OprInt>(fst.var).second));
                    return;
                }
                throw std::runtime_error("cannot apply " + std::get<0>(fst.translate()) + " on: " + std::get<0>(snd.translate()));
            case Token::CmpInt:
                if (snd.calc(), snd.var.index() == Token::Int) {
                    static const auto T = Tree(std::in_place_index<Token::Fun>, Node<std::pair<std::string, Tree>>::make("T", Tree(std::in_place_index<Token::Fun>, Node<std::pair<std::string, Tree>>::make("F", Tree(std::in_place_index<Token::Par>, "T")))));
                    static const auto F = Tree(std::in_place_index<Token::Fun>, Node<std::pair<std::string, Tree>>::make("T", Tree(std::in_place_index<Token::Fun>, Node<std::pair<std::string, Tree>>::make("F", Tree(std::in_place_index<Token::Par>, "F")))));
                    *this = std::get<Token::CmpInt>(fst.var).first.second(std::move(std::get<Token::Int>(snd.var)), std::move(std::get<Token::CmpInt>(fst.var).second)) ? T : F;
                    return;
                }
                throw std::runtime_error("cannot apply " + std::get<0>(fst.translate()) + " on: " + std::get<0>(snd.translate()));
            default:
                throw std::runtime_error("invalid function: " + std::get<0>(fst.translate()));
            }
        }
        case Token::Arg: {
            auto &arg = std::get<Token::Arg>(var);
            if (not arg->second) {
                arg->first.calc();
                arg->second = true;
            }
            if (arg.use_count() == 1) {
                *this = Tree(std::move(arg->first));
            } else {
                *this = arg->first;
            }
        } break;
        case Token::Def:
            if (auto const &it = dct.find(std::get<Token::Def>(var)); it != dct.end()) {
                *this = it->second;
                return calc();
            }
            throw std::runtime_error("undefined symbol: &" + std::get<Token::Def>(var));
        case Token::Par:
            throw std::runtime_error("unbound variable: $" + std::get<Token::Par>(var));
        }
    }
    void substitute(std::shared_ptr<std::pair<Tree, bool>> const &arg, std::string const &par) {
        switch (var.index()) {
        case Token::App:
            std::get<Token::App>(var)->first.substitute(arg, par);
            std::get<Token::App>(var)->second.substitute(arg, par);
            return;
        case Token::Fun:
            if (std::get<Token::Fun>(var)->first != par) {
                std::get<Token::Fun>(var)->second.substitute(arg, par);
            }
            return;
        case Token::Out:
            if (std::get<Token::Out>(var)->first != par) {
                std::get<Token::Out>(var)->second.substitute(arg, par);
            }
            return;
        case Token::Par:
            if (std::get<Token::Par>(var) == par) {
                var.emplace<Token::Arg>(arg);
            }
            return;
        }
    }
    static inline std::unordered_map<std::string, Tree> dct;
public:
    static Tree cal(Slice &&exp) {
        auto res = parse(std::move(exp));
        res.calc();
        return res;
    }
    static void def(Slice &&exp, std::string const &key) {
        dct.insert_or_assign(key, parse(std::move(exp)));
    }
    static auto const &dir() {
        return dct;
    }
    static void clr() {
        return dct.clear();
    }
    std::tuple<std::string, bool, bool> translate() const {
        switch (var.index()) {
        case Token::Fun:
            return {"\\" + std::get<Token::Fun>(var)->first + " " + std::get<0>(std::get<Token::Fun>(var)->second.translate()), 1, 0};
        case Token::Out:
            return {"^" + std::get<Token::Out>(var)->first + " " + std::get<0>(std::get<Token::Out>(var)->second.translate()), 1, 0};
        case Token::Par:
            return {"$" + std::get<Token::Par>(var), 0, 0};
        case Token::Def:
            return {"&" + std::get<Token::Def>(var), 0, 0};
        case Token::Int:
            return {std::get<Token::Int>(var).to_string(), 0, 0};
        case Token::Opr:
            return {std::string{std::get<Token::Opr>(var).first}, 0, 0};
        case Token::Cmp:
            return {std::string{std::get<Token::Cmp>(var).first}, 0, 0};
        case Token::OprInt:
            return {std::string{std::get<Token::OprInt>(var).first.first, ' '} + std::get<Token::OprInt>(var).second.to_string(), 0, 1};
        case Token::CmpInt:
            return {std::string{std::get<Token::CmpInt>(var).first.first, ' '} + std::get<Token::CmpInt>(var).second.to_string(), 0, 1};
        case Token::App: {
            auto [lss, lbl, lbr] = std::get<Token::App>(var)->first.translate();
            auto [rss, rbl, rbr] = std::get<Token::App>(var)->second.translate();
            return {(lbl ? "(" + std::move(lss) + ")" : std::move(lss)) + " " + (rbr ? "(" + std::move(rss) + ")" : std::move(rss)), rbl && not rbr, 1};
        }
        case Token::Arg:
            return std::get<Token::Arg>(var)->first.translate();
        default:
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
            } else if (cmd[0] == '&') {
                Tree::def(std::move(exp), cmd(1, 0));
            } else if (cmd.size() == 3 && cmd == "cal") {
                auto res = Tree::cal(std::move(exp));
                std::cerr << ps_res;
                std::cout << std::get<0>(res.translate()) << std::endl;
            } else if (cmd.size() == 3 && cmd == "dir") {
                for (auto const &[key, val] : Tree::dir()) {
                    std::cerr << ps_out;
                    std::cout << std::left << std::setw(10) << "&" + (key.size() <= 8 ? key : key.substr(0, 6) + "..") << std::get<0>(val.translate()) << std::endl;
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

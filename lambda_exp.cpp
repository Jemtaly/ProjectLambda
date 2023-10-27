#include <iomanip>
#include <iostream>
#include <optional>
#include <variant>
#include <cassert>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "slice.hpp"
#ifndef USE_GMP
#include "strint.hpp"
#else
#include "gmpint.hpp"
#endif
#if defined _WIN32
#include <Windows.h>
#elif defined __unix__
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#endif
#ifndef STACK_SIZE
#define STACK_SIZE 8388608 // 8 MiB
#endif
char *stack_top;
char *stack_cur;
void ini_stack() {
    char dummy;
    stack_top = &dummy;
}
bool chk_stack() {
    char dummy;
    stack_cur = &dummy;
    return stack_top - stack_cur >= STACK_SIZE / 2;
}
#if defined _WIN32
void clr_flag() {
    GetAsyncKeyState(VK_ESCAPE);
}
bool chk_flag() {
    return GetAsyncKeyState(VK_ESCAPE) & 0x0001;
}
#elif defined __unix__
bool flag_rec;
void set_flag(int) {
    flag_rec = 1;
}
void clr_flag() {
    flag_rec = 0;
}
bool chk_flag() {
    return flag_rec;
}
#endif
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
    enum TokenIdx: std::size_t {
        Nil, Chk,
        Par, Int,
        Opr, AOI,
        Cmp, ACI,
        LEF, EEF, // Lazy/Eager-Evaluation Function
        App,
    };
    using TokenVar = std::variant<
        std::monostate, std::monostate, std::string, StrInt,
        std::pair<char, opr_t>, std::pair<std::pair<char, opr_t>, StrInt>,
        std::pair<char, cmp_t>, std::pair<std::pair<char, cmp_t>, StrInt>,
        std::pair<std::string, Tree>, std::pair<std::string, Tree>,
        std::pair<Tree, Tree>>;
    std::shared_ptr<TokenVar> sp; bool lb;
    Tree(TokenVar *ptr): sp(ptr), lb(0) {}
    static Tree first(Tree &&fst) {
        if (fst.sp == nullptr) {
            throw std::runtime_error("empty expression");
        } else {
            return std::move(fst);
        }
    }
    static Tree build(Tree &&fst, Tree &&snd) {
        if (fst.sp == nullptr) {
            return std::move(snd);
        } else {
            return new TokenVar(std::in_place_index<TokenIdx::App>, std::move(fst), std::move(snd));
        }
    }
    static Tree parse(Slice &&exp, Tree &&fun = nullptr, Tree &&fst = nullptr) {
        if (auto sym = read(exp); sym.empty()) {
            return build(std::move(fun), first(std::move(fst)));
        } else if (sym[0] == '\\') {
            return build(std::move(fun), build(std::move(fst), new TokenVar(std::in_place_index<TokenIdx::LEF>, sym(1, 0), parse(std::move(exp)))));
        } else if (sym[0] == '|') {
            return parse(std::move(exp), new TokenVar(std::in_place_index<TokenIdx::LEF>, sym(1, 0), build(std::move(fun), first(std::move(fst)))));
        } else if (sym[0] == '^') {
            return build(std::move(fun), build(std::move(fst), new TokenVar(std::in_place_index<TokenIdx::EEF>, sym(1, 0), parse(std::move(exp)))));
        } else if (sym[0] == '@') {
            return parse(std::move(exp), new TokenVar(std::in_place_index<TokenIdx::EEF>, sym(1, 0), build(std::move(fun), first(std::move(fst)))));
        } else {
            return parse(std::move(exp), std::move(fun), build(std::move(fst), lex(std::move(sym))));
        }
    }
    static Tree lex(Slice const &sym) {
        if (sym[0] == '(' && sym[-1] == ')') {
            return parse(sym(1, -1));
        } else if (sym[0] == '$') {
            return new TokenVar(std::in_place_index<TokenIdx::Par>, sym(1, 0));
        } else if (sym.size() == 3 && sym == "...") {
            return new TokenVar(std::in_place_index<TokenIdx::Nil>);
        } else if (sym.size() == 1 && sym == "?") {
            return new TokenVar(std::in_place_index<TokenIdx::Chk>);
        } else if (auto const &o = oprs.find(sym[0]); sym.size() == 1 && o != oprs.end()) {
            return new TokenVar(std::in_place_index<TokenIdx::Opr>, *o);
        } else if (auto const &c = cmps.find(sym[0]); sym.size() == 1 && c != cmps.end()) {
            return new TokenVar(std::in_place_index<TokenIdx::Cmp>, *c);
        } else try {
            return new TokenVar(std::in_place_index<TokenIdx::Int>, StrInt::from_string(sym));
        } catch (...) {
            throw std::runtime_error("invalid symbol: " + std::string(sym));
        }
    }
    void calc() {
        static const auto T = Tree(new TokenVar(std::in_place_index<TokenIdx::LEF>, "T", Tree(new TokenVar(std::in_place_index<TokenIdx::LEF>, "F", Tree(new TokenVar(std::in_place_index<TokenIdx::Par>, "T"))))));
        static const auto F = Tree(new TokenVar(std::in_place_index<TokenIdx::LEF>, "T", Tree(new TokenVar(std::in_place_index<TokenIdx::LEF>, "F", Tree(new TokenVar(std::in_place_index<TokenIdx::Par>, "F"))))));
        static const auto N = Tree(new TokenVar(std::in_place_index<TokenIdx::Nil>));
        if (chk_stack()) {
            throw std::runtime_error("recursion limit exceeded");
        }
        if (chk_flag()) {
            throw std::runtime_error("keyboard interrupt");
        }
        if (auto papp = std::get_if<TokenIdx::App>(sp.get())) {
            auto &[fst, snd] = *papp;
            fst.calc();
            snd.lb = 1;
            if (auto pnil = std::get_if<TokenIdx::Nil>(fst.sp.get())) {
                *sp = *N.sp;
            } else if (auto pchk = std::get_if<TokenIdx::Chk>(fst.sp.get())) {
                snd.calc();
                *sp = snd.sp->index() == TokenIdx::Nil ? *F.sp : *T.sp;
            } else if (auto plef = std::get_if<TokenIdx::LEF>(fst.sp.get())) {
                auto &[par, tmp] = *plef;
                auto t_s = tmp.substitute(snd, par);
                t_s.calc();
                *sp = *t_s.sp;
            } else if (auto peef = std::get_if<TokenIdx::EEF>(fst.sp.get())) {
                snd.calc();
                auto &[par, tmp] = *peef;
                auto t_s = tmp.substitute(snd, par);
                t_s.calc();
                *sp = *t_s.sp;
            } else if (auto popr = std::get_if<TokenIdx::Opr>(fst.sp.get())) {
                snd.calc();
                if (auto pint = std::get_if<TokenIdx::Int>(snd.sp.get()); pint && (*pint || popr->first != '/' && popr->first != '%')) {
                    *sp = std::make_pair(*popr, *pint);
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(snd.sp.get())) {
                    *sp = *N.sp;
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto pcmp = std::get_if<TokenIdx::Cmp>(fst.sp.get())) {
                snd.calc();
                if (auto pint = std::get_if<TokenIdx::Int>(snd.sp.get())) {
                    *sp = std::make_pair(*pcmp, *pint);
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(snd.sp.get())) {
                    *sp = *N.sp;
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto paoi = std::get_if<TokenIdx::AOI>(fst.sp.get())) {
                snd.calc();
                if (auto pint = std::get_if<TokenIdx::Int>(snd.sp.get())) {
                    *sp = paoi->first.second(*pint, paoi->second);
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(snd.sp.get())) {
                    *sp = *N.sp;
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto paci = std::get_if<TokenIdx::ACI>(fst.sp.get())) {
                snd.calc();
                if (auto pint = std::get_if<TokenIdx::Int>(snd.sp.get())) {
                    *sp = paci->first.second(*pint, paci->second) ? *T.sp : *F.sp;
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(snd.sp.get())) {
                    *sp = *N.sp;
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else {
                throw std::runtime_error("invalid function: " + fst.translate());
            }
        }
    }
    Tree substitute(Tree const &arg, std::string const &tar) {
        if (lb) {
            return *this;
        }
        if (auto papp = std::get_if<TokenIdx::App>(sp.get())) {
            auto &[fst, snd] = *papp;
            auto f_s = fst.substitute(arg, tar);
            auto s_s = snd.substitute(arg, tar);
            if (f_s.sp != fst.sp || s_s.sp != snd.sp) {
                return new TokenVar(std::in_place_index<TokenIdx::App>, std::move(f_s), std::move(s_s));
            }
        } else if (auto plef = std::get_if<TokenIdx::LEF>(sp.get())) {
            auto &[par, tmp] = *plef;
            if (par != tar) {
                auto t_s = tmp.substitute(arg, tar);
                if (t_s.sp != tmp.sp) {
                    return new TokenVar(std::in_place_index<TokenIdx::LEF>, par, std::move(t_s));
                }
            }
        } else if (auto peef = std::get_if<TokenIdx::EEF>(sp.get())) {
            auto &[par, tmp] = *peef;
            if (par != tar) {
                auto t_s = tmp.substitute(arg, tar);
                if (t_s.sp != tmp.sp) {
                    return new TokenVar(std::in_place_index<TokenIdx::EEF>, par, std::move(t_s));
                }
            }
        } else if (auto ppar = std::get_if<TokenIdx::Par>(sp.get())) {
            if (*ppar == tar) {
                return arg;
            }
        }
        return *this;
    }
    void substitute(std::unordered_map<std::string, Tree> &map) {
        if (auto papp = std::get_if<TokenIdx::App>(sp.get())) {
            auto &[fst, snd] = *papp;
            fst.substitute(map);
            snd.substitute(map);
        } else if (auto plef = std::get_if<TokenIdx::LEF>(sp.get())) {
            auto &[par, tmp] = *plef;
            if (auto const &it = map.find(par); it != map.end()) {
                auto sav = std::move(it->second);
                tmp.substitute(map);
                it->second = std::move(sav);
            } else {
                tmp.substitute(map);
            }
        } else if (auto peef = std::get_if<TokenIdx::EEF>(sp.get())) {
            auto &[par, tmp] = *peef;
            if (auto const &it = map.find(par); it != map.end()) {
                auto sav = std::move(it->second);
                tmp.substitute(map);
                it->second = std::move(sav);
            } else {
                tmp.substitute(map);
            }
        } else if (auto ppar = std::get_if<TokenIdx::Par>(sp.get())) {
            if (auto const &it = map.find(*ppar); it != map.end() && it->second.sp) {
                *this = it->second;
            }
        }
    }
    void analyze(std::unordered_set<std::string> &set) const {
        if (auto papp = std::get_if<TokenIdx::App>(sp.get())) {
            auto &[fst, snd] = *papp;
            fst.analyze(set);
            snd.analyze(set);
        } else if (auto plef = std::get_if<TokenIdx::LEF>(sp.get())) {
            auto &[par, tmp] = *plef;
            if (auto const &it = set.find(par); it != set.end()) {
                tmp.analyze(set);
            } else {
                auto jt = set.insert(par).first;
                tmp.analyze(set);
                set.erase(jt);
            }
        } else if (auto peef = std::get_if<TokenIdx::EEF>(sp.get())) {
            auto &[par, tmp] = *peef;
            if (auto const &it = set.find(par); it != set.end()) {
                tmp.analyze(set);
            } else {
                auto jt = set.insert(par).first;
                tmp.analyze(set);
                set.erase(jt);
            }
        } else if (auto ppar = std::get_if<TokenIdx::Par>(sp.get())) {
            if (auto const &it = set.find(*ppar); it == set.end()) {
                throw std::runtime_error("unbound variable: $" + *ppar);
            }
        }
    }
    static inline std::unordered_map<std::string, Tree> map;
public:
    Tree(Tree const &other) = default;
    Tree &operator=(Tree const &) = default;
    Tree(Tree &&) = default;
    Tree &operator=(Tree &&) = default;
    ~Tree() = default;
    static auto const &put(Slice &&exp, std::string const &par, bool calc) {
        auto res = parse(std::move(exp));
        res.substitute(map);
        std::unordered_set<std::string> set;
        res.analyze(set);
        map.erase(par);
        if (calc) {
            clr_flag();
            res.calc();
        }
        return map.emplace(par, res).first->second;
    }
    static auto const &dir() {
        return map;
    }
    static void clr() {
        return map.clear();
    }
    std::string translate(bool lb = 0, bool rb = 0) const {
        if (auto pnil = std::get_if<TokenIdx::Nil>(sp.get())) {
            return "...";
        } else if (auto pchk = std::get_if<TokenIdx::Chk>(sp.get())) {
            return "?";
        } else if (auto plef = std::get_if<TokenIdx::LEF>(sp.get())) {
            auto &[par, tmp] = *plef;
            auto s = "\\" + par + " " + tmp.translate(0, rb && !rb);
            return rb ? "(" + s + ")" : s;
        } else if (auto peef = std::get_if<TokenIdx::EEF>(sp.get())) {
            auto &[par, tmp] = *peef;
            auto s = "^" + par + " " + tmp.translate(0, rb && !rb);
            return rb ? "(" + s + ")" : s;
        } else if (auto pint = std::get_if<TokenIdx::Int>(sp.get())) {
            return pint->to_string();
        } else if (auto popr = std::get_if<TokenIdx::Opr>(sp.get())) {
            return std::string{popr->first};
        } else if (auto pcmp = std::get_if<TokenIdx::Cmp>(sp.get())) {
            return std::string{pcmp->first};
        } else if (auto paoi = std::get_if<TokenIdx::AOI>(sp.get())) {
            auto s = std::string{paoi->first.first, ' '} + paoi->second.to_string();
            return lb ? "(" + s + ")" : s;
        } else if (auto paci = std::get_if<TokenIdx::ACI>(sp.get())) {
            auto s = std::string{paci->first.first, ' '} + paci->second.to_string();
            return lb ? "(" + s + ")" : s;
        } else if (auto papp = std::get_if<TokenIdx::App>(sp.get())) {
            auto &[fst, snd] = *papp;
            auto s = fst.translate(lb && !lb, 1) + " " + snd.translate(1, rb && !lb);
            return lb ? "(" + s + ")" : s;
        } else if (auto ppar = std::get_if<TokenIdx::Par>(sp.get())) {
            return "$" + *ppar;
        } else {
            assert(false); // unreachable
        }
    }
};
int main(int argc, char *argv[]) {
    ini_stack();
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
    struct sigaction act;
    act.sa_handler = set_flag;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0; // use SA_RESTART to avoid getting EOF when SIGINT is received during input
    sigaction(SIGINT, &act, NULL);
#endif
    std::string ps_in = check_stderr && check_stdin ? ">> " : "";
    std::string ps_out = check_stderr && check_stdout ? "=> " : "";
    std::string ps_res = check_stderr && check_stdout ? "== " : "";
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
                for (auto &[par, arg] : Tree::dir()) {
                    std::cerr << ps_out;
                    std::cout << ":" + par << std::endl;
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

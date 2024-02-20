#include <iomanip>
#include <iostream>
#include <optional>
#include <variant>
#include <cassert>
#include <string>
#include <memory>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "container.hpp"
#include "slice.hpp"
#ifndef USE_GMP
#include "bigint_nat.hpp" // native big integer
#else
#include "bigint_gmp.hpp" // GNU MP big integer
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
BigInt operator+(BigInt const &lval, BigInt const &rval);
BigInt operator-(BigInt const &lval, BigInt const &rval);
BigInt operator*(BigInt const &lval, BigInt const &rval);
BigInt operator/(BigInt const &lval, BigInt const &rval);
BigInt operator%(BigInt const &lval, BigInt const &rval);
bool operator>(BigInt const &lval, BigInt const &rval);
bool operator<(BigInt const &lval, BigInt const &rval);
bool operator>=(BigInt const &lval, BigInt const &rval);
bool operator<=(BigInt const &lval, BigInt const &rval);
bool operator==(BigInt const &lval, BigInt const &rval);
bool operator!=(BigInt const &lval, BigInt const &rval);
typedef BigInt (*opr_t)(BigInt const &, BigInt const &);
typedef bool (*cmp_t)(BigInt const &, BigInt const &);
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
        Und,
        Nil, Chk,
        Par, Int,
        Opr, AOI,
        Cmp, ACI,
        LEF, EEF, // Lazy/Eager-Evaluation Function
        App, Arg, Glb,
    };
    using TokenVar = std::variant<
        std::nullopt_t,
        std::monostate, std::monostate,
        std::string, BigInt,
        std::pair<char, opr_t>, std::pair<std::pair<char, opr_t>, BigInt>,
        std::pair<char, cmp_t>, std::pair<std::pair<char, cmp_t>, BigInt>,
        Box<std::pair<std::string, Tree>>, Box<std::pair<std::string, Tree>>,
        Box<std::pair<Tree, Tree>>, std::shared_ptr<std::pair<Tree, bool>>, std::string>;
    TokenVar token;
    template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<TokenVar, Args &&...>>>
    Tree(Args &&...args): token(std::forward<Args>(args)...) {}
    static Tree first(Tree &&fst) {
        if (fst.token.index() == TokenIdx::Und) {
            throw std::runtime_error("empty expression");
        } else {
            return std::move(fst);
        }
    }
    static Tree build(Tree &&fst, Tree &&snd) {
        if (fst.token.index() == TokenIdx::Und) {
            return std::move(snd);
        } else {
            return Box<std::pair<Tree, Tree>>::make(std::move(fst), std::move(snd));
        }
    }
    static Tree parse(Slice &&exp, Tree &&fun = std::nullopt, Tree &&fst = std::nullopt) {
        if (auto sym = read(exp); sym.empty()) {
            return build(std::move(fun), first(std::move(fst)));
        } else if (sym[0] == '\\') {
            return build(std::move(fun), build(std::move(fst), Tree(std::in_place_index<TokenIdx::LEF>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp))))));
        } else if (sym[0] == '|') {
            return parse(std::move(exp), Tree(std::in_place_index<TokenIdx::LEF>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), build(std::move(fun), first(std::move(fst))))));
        } else if (sym[0] == '^') {
            return build(std::move(fun), build(std::move(fst), Tree(std::in_place_index<TokenIdx::EEF>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp))))));
        } else if (sym[0] == '@') {
            return parse(std::move(exp), Tree(std::in_place_index<TokenIdx::EEF>, Box<std::pair<std::string, Tree>>::make(sym(1, 0), build(std::move(fun), first(std::move(fst))))));
        } else {
            return parse(std::move(exp), std::move(fun), build(std::move(fst), lex(std::move(sym))));
        }
    }
    static Tree lex(Slice const &sym) {
        if (sym[0] == '(' && sym[-1] == ')') {
            return parse(sym(1, -1));
        } else if (sym[0] == '$') {
            return Tree(std::in_place_index<TokenIdx::Par>, sym(1, 0));
        } else if (sym[0] == '&') {
            return Tree(std::in_place_index<TokenIdx::Glb>, sym(1, 0));
        } else if (sym.size() == 3 && sym == "...") {
            return Tree(std::in_place_index<TokenIdx::Nil>);
        } else if (sym.size() == 1 && sym == "?") {
            return Tree(std::in_place_index<TokenIdx::Chk>);
        } else if (auto const &o = oprs.find(sym[0]); sym.size() == 1 && o != oprs.end()) {
            return Tree(std::in_place_index<TokenIdx::Opr>, *o);
        } else if (auto const &c = cmps.find(sym[0]); sym.size() == 1 && c != cmps.end()) {
            return Tree(std::in_place_index<TokenIdx::Cmp>, *c);
        } else try {
            return Tree(std::in_place_index<TokenIdx::Int>, BigInt::from_string(sym));
        } catch (...) {
            throw std::runtime_error("invalid symbol: " + std::string(sym));
        }
    }
    void calc() {
        static const auto T = Tree(std::in_place_index<TokenIdx::LEF>, Box<std::pair<std::string, Tree>>::make("T", Tree(std::in_place_index<TokenIdx::LEF>, Box<std::pair<std::string, Tree>>::make("F", Tree(std::in_place_index<TokenIdx::Par>, "T")))));
        static const auto F = Tree(std::in_place_index<TokenIdx::LEF>, Box<std::pair<std::string, Tree>>::make("T", Tree(std::in_place_index<TokenIdx::LEF>, Box<std::pair<std::string, Tree>>::make("F", Tree(std::in_place_index<TokenIdx::Par>, "F")))));
        if (chk_stack()) {
            throw std::runtime_error("recursion limit exceeded");
        }
    tail_call:
        if (chk_flag()) {
            throw std::runtime_error("keyboard interrupt");
        }
        if (auto papp = std::get_if<TokenIdx::App>(&token)) {
            auto &[fst, snd] = **papp;
            fst.calc();
            if (auto pnil = std::get_if<TokenIdx::Nil>(&fst.token)) {
                token.emplace<TokenIdx::Nil>();
            } else if (auto pchk = std::get_if<TokenIdx::Chk>(&fst.token)) {
                snd.calc();
                *this = snd.token.index() == TokenIdx::Nil ? F : T;
            } else if (auto plef = std::get_if<TokenIdx::LEF>(&fst.token)) {
                auto &[par, tmp] = **plef;
                tmp.substitute(std::make_shared<std::pair<Tree, bool>>(std::move(snd), 0), std::move(par));
                *this = Tree(std::move(tmp));
                goto tail_call;
            } else if (auto peef = std::get_if<TokenIdx::EEF>(&fst.token)) {
                snd.calc();
                auto &[par, tmp] = **peef;
                tmp.substitute(std::make_shared<std::pair<Tree, bool>>(std::move(snd), 1), std::move(par));
                *this = Tree(std::move(tmp));
                goto tail_call;
            } else if (auto popr = std::get_if<TokenIdx::Opr>(&fst.token)) {
                snd.calc();
                if (auto pint = std::get_if<TokenIdx::Int>(&snd.token); pint && (*pint || popr->first != '/' && popr->first != '%')) {
                    token = std::make_pair(std::move(*popr), std::move(*pint));
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(&snd.token)) {
                    token.emplace<TokenIdx::Nil>();
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto pcmp = std::get_if<TokenIdx::Cmp>(&fst.token)) {
                snd.calc();
                if (auto pint = std::get_if<TokenIdx::Int>(&snd.token)) {
                    token = std::make_pair(std::move(*pcmp), std::move(*pint));
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(&snd.token)) {
                    token.emplace<TokenIdx::Nil>();
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto paoi = std::get_if<TokenIdx::AOI>(&fst.token)) {
                snd.calc();
                if (auto pint = std::get_if<TokenIdx::Int>(&snd.token)) {
                    token = paoi->first.second(std::move(*pint), std::move(paoi->second));
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(&snd.token)) {
                    token.emplace<TokenIdx::Nil>();
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto paci = std::get_if<TokenIdx::ACI>(&fst.token)) {
                snd.calc();
                if (auto pint = std::get_if<TokenIdx::Int>(&snd.token)) {
                    *this = paci->first.second(std::move(*pint), std::move(paci->second)) ? T : F;
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(&snd.token)) {
                    token.emplace<TokenIdx::Nil>();
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else {
                throw std::runtime_error("invalid function: " + fst.translate());
            }
        } else if (auto parg = std::get_if<TokenIdx::Arg>(&token)) {
            auto &shr = (*parg)->first;
            auto rec = (*parg)->second;
            if (parg->use_count() == 1) {
                *this = Tree(std::move(shr));
                if (not rec) {
                    goto tail_call;
                }
            } else {
                if (not rec) {
                    shr.calc();
                    rec = true;
                }
                *this = shr;
            }
        } else if (auto pglb = std::get_if<TokenIdx::Glb>(&token)) {
            if (auto const &itr = map.find(*pglb); itr != map.end()) {
                *this = itr->second;
                goto tail_call;
            } else {
                throw std::runtime_error("undefined symbol: &" + *pglb);
            }
        }
    }
    void substitute(std::shared_ptr<std::pair<Tree, bool>> const &arg, std::string const &tar) {
        if (auto papp = std::get_if<TokenIdx::App>(&token)) {
            auto &[fst, snd] = **papp;
            fst.substitute(arg, tar);
            snd.substitute(arg, tar);
        } else if (auto plef = std::get_if<TokenIdx::LEF>(&token)) {
            auto &[par, tmp] = **plef;
            if (par != tar) {
                tmp.substitute(arg, tar);
            }
        } else if (auto peef = std::get_if<TokenIdx::EEF>(&token)) {
            auto &[par, tmp] = **peef;
            if (par != tar) {
                tmp.substitute(arg, tar);
            }
        } else if (auto ppar = std::get_if<TokenIdx::Par>(&token)) {
            if (*ppar == tar) {
                token.emplace<TokenIdx::Arg>(arg);
            }
        }
    }
    void analyze(std::unordered_set<std::string> &set) const {
        if (auto papp = std::get_if<TokenIdx::App>(&token)) {
            auto &[fst, snd] = **papp;
            fst.analyze(set);
            snd.analyze(set);
        } else if (auto plef = std::get_if<TokenIdx::LEF>(&token)) {
            auto &[par, tmp] = **plef;
            if (set.find(par) != set.end()) {
                tmp.analyze(set);
            } else {
                auto pos = set.insert(par).first;
                tmp.analyze(set);
                set.erase(pos);
            }
        } else if (auto peef = std::get_if<TokenIdx::EEF>(&token)) {
            auto &[par, tmp] = **peef;
            if (set.find(par) != set.end()) {
                tmp.analyze(set);
            } else {
                auto pos = set.insert(par).first;
                tmp.analyze(set);
                set.erase(pos);
            }
        } else if (auto ppar = std::get_if<TokenIdx::Par>(&token)) {
            if (set.find(*ppar) == set.end()) {
                throw std::runtime_error("unbound variable: $" + *ppar);
            }
        }
    }
    static inline std::unordered_map<std::string, Tree> map;
public:
    Tree(Tree const &other): token(other.token) {}
    Tree &operator=(Tree const &other) {
        token = other.token;
        return *this;
    }
    Tree(Tree &&other): token(std::exchange(other.token, std::nullopt)) {}
    Tree &operator=(Tree &&other) {
        token = std::exchange(other.token, std::nullopt);
        return *this;
    }
    ~Tree() {
        if (token.index() == TokenIdx::Und) {
            return;
        }
        std::queue<TokenVar> flat;
        flat.push(std::exchange(token, std::nullopt));
        for (; not flat.empty(); flat.pop()) {
            auto &token = flat.front();
            if (auto papp = std::get_if<TokenIdx::App>(&token)) {
                auto &[fst, snd] = **papp;
                flat.push(std::exchange(fst.token, std::nullopt));
                flat.push(std::exchange(snd.token, std::nullopt));
            } else if (auto parg = std::get_if<TokenIdx::LEF>(&token)) {
                auto &[par, tmp] = **parg;
                flat.push(std::exchange(tmp.token, std::nullopt));
            } else if (auto parg = std::get_if<TokenIdx::EEF>(&token)) {
                auto &[par, tmp] = **parg;
                flat.push(std::exchange(tmp.token, std::nullopt));
            } else if (auto parg = std::get_if<TokenIdx::Arg>(&token)) {
                auto &shr = (*parg)->first;
                if (parg->use_count() == 1) {
                    flat.push(std::exchange(shr.token, std::nullopt));
                }
            }
        }
    }
    static auto cal(Slice &&exp) {
        auto res = parse(std::move(exp));
        std::unordered_set<std::string> set;
        res.analyze(set);
        clr_flag();
        res.calc();
        return res;
    }
    static void def(Slice &&exp, std::string const &glb) {
        std::unordered_set<std::string> set;
        auto res = parse(std::move(exp));
        res.analyze(set);
        map.insert_or_assign(glb, std::move(res));
    }
    static auto const &dir() {
        return map;
    }
    static void clr() {
        return map.clear();
    }
    std::string translate(bool lb = 0, bool rb = 0) const {
        if (auto pnil = std::get_if<TokenIdx::Nil>(&token)) {
            return "...";
        } else if (auto pchk = std::get_if<TokenIdx::Chk>(&token)) {
            return "?";
        } else if (auto plef = std::get_if<TokenIdx::LEF>(&token)) {
            auto &[par, tmp] = **plef;
            auto s = "\\" + par + " " + tmp.translate(0, rb && !rb);
            return rb ? "(" + s + ")" : s;
        } else if (auto peef = std::get_if<TokenIdx::EEF>(&token)) {
            auto &[par, tmp] = **peef;
            auto s = "^" + par + " " + tmp.translate(0, rb && !rb);
            return rb ? "(" + s + ")" : s;
        } else if (auto pint = std::get_if<TokenIdx::Int>(&token)) {
            return pint->to_string();
        } else if (auto popr = std::get_if<TokenIdx::Opr>(&token)) {
            return std::string{popr->first};
        } else if (auto pcmp = std::get_if<TokenIdx::Cmp>(&token)) {
            return std::string{pcmp->first};
        } else if (auto paoi = std::get_if<TokenIdx::AOI>(&token)) {
            auto s = std::string{paoi->first.first, ' '} + paoi->second.to_string();
            return lb ? "(" + s + ")" : s;
        } else if (auto paci = std::get_if<TokenIdx::ACI>(&token)) {
            auto s = std::string{paci->first.first, ' '} + paci->second.to_string();
            return lb ? "(" + s + ")" : s;
        } else if (auto papp = std::get_if<TokenIdx::App>(&token)) {
            auto &[fst, snd] = **papp;
            auto s = fst.translate(lb && !lb, 1) + " " + snd.translate(1, rb && !lb);
            return lb ? "(" + s + ")" : s;
        } else if (auto parg = std::get_if<TokenIdx::Arg>(&token)) {
            return (*parg)->first.translate(lb, rb);
        } else if (auto ppar = std::get_if<TokenIdx::Par>(&token)) {
            return "$" + *ppar;
        } else if (auto pglb = std::get_if<TokenIdx::Glb>(&token)) {
            return "&" + *pglb;
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
                Tree::def(std::move(exp), cmd(1, 0));
            } else if (cmd.size() == 3 && cmd == "cal") {
                auto res = Tree::cal(std::move(exp));
                std::cerr << ps_res;
                std::cout << res.translate() << std::endl;
            } else if (cmd.size() == 3 && cmd == "dir") {
                for (auto &[glb, tmp] : Tree::dir()) {
                    std::cerr << ps_out;
                    std::cout << std::left << std::setw(10) << ":" + (glb.size() <= 8 ? glb : glb.substr(0, 6) + "..") << tmp.translate() << std::endl;
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

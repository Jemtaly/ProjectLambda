#include <cassert>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <container.hpp>

#ifndef USE_GMP
#include <bigint_nat.hpp>  // native big integer
#else
#include <bigint_gmp.hpp>  // GNU MP big integer
#endif

#if defined _WIN32
#include <Windows.h>
#elif defined __unix__
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

struct StreamStatus {
    bool stdin_flag;
    bool stdout_flag;
    bool stderr_flag;
};

StreamStatus io_check() {
    bool stdin_flag = false;
    bool stdout_flag = false;
    bool stderr_flag = false;
#if defined _WIN32
    DWORD dwModeTemp;
    stdin_flag = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &dwModeTemp);
    stdout_flag = GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &dwModeTemp);
    stderr_flag = GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &dwModeTemp);
#elif defined __unix__
    stdin_flag = isatty(fileno(stdin));
    stdout_flag = isatty(fileno(stdout));
    stderr_flag = isatty(fileno(stderr));
#endif
    return {
        stdin_flag,
        stdout_flag,
        stderr_flag,
    };
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

__attribute__((constructor)) void set_signal() {
    // set signal handler
    struct sigaction act;
    act.sa_handler = set_flag;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;  // use SA_RESTART to avoid getting EOF when SIGINT is received during input
    sigaction(SIGINT, &act, NULL);
}
#endif

std::string_view read(std::string_view &exp) {
    size_t i = 0;
    size_t n = exp.size();
    for (;; i++) {
        if (i == n) {
            auto res = exp.substr(i, n - i);
            exp.remove_prefix(i);
            return res;
        } else if (exp[i] != ' ') {
            break;
        }
    }
    size_t j = i;
    size_t c = 0;
    for (;; i++) {
        if ((i == n || exp[i] == ' ') && c == 0) {
            auto res = exp.substr(j, i - j);
            exp.remove_prefix(i);
            return res;
        } else if (i == n) {
            throw std::runtime_error("mismatched parentheses");
        } else if (exp[i] == '(') {
            c++;
        } else if (exp[i] == ')') {
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
    enum TokenIdx : std::size_t {
        Und,
        Nil, Chk,
        Par, Int,
        Opr, AOI,
        Cmp, ACI,
        Fun,  // Lazy/Eager-Evaluation Function
        App, Arg,
    };

    using TokenVar = std::variant<
        std::nullopt_t,
        std::monostate, std::monostate,
        std::string, BigInt,
        std::pair<char, opr_t>, std::pair<std::pair<char, opr_t>, BigInt>,
        std::pair<char, cmp_t>, std::pair<std::pair<char, cmp_t>, BigInt>,
        Box<std::tuple<std::string, Tree, bool>>,
        Box<std::pair<Tree, Tree>>, std::shared_ptr<std::pair<Tree, bool>>>;

    TokenVar token;

    template<typename... Args, typename = std::enable_if_t<std::is_constructible_v<TokenVar, Args &&...>>>
    Tree(Args &&...args)
        : token(std::forward<Args>(args)...) {}

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

    static Tree parse(std::string_view &&exp, Tree &&fun = std::nullopt, Tree &&fst = std::nullopt) {
        if (auto sym = read(exp); sym.empty()) {
            return build(std::move(fun), first(std::move(fst)));
        } else if (sym.front() == '\\') {
            return build(std::move(fun), build(std::move(fst), Tree(std::in_place_index<TokenIdx::Fun>, Box<std::tuple<std::string, Tree, bool>>::make(sym.substr(1), parse(std::move(exp)), 0))));
        } else if (sym.front() == '|') {
            return parse(std::move(exp), Tree(std::in_place_index<TokenIdx::Fun>, Box<std::tuple<std::string, Tree, bool>>::make(sym.substr(1), build(std::move(fun), first(std::move(fst))), 0)));
        } else if (sym.front() == '^') {
            return build(std::move(fun), build(std::move(fst), Tree(std::in_place_index<TokenIdx::Fun>, Box<std::tuple<std::string, Tree, bool>>::make(sym.substr(1), parse(std::move(exp)), 1))));
        } else if (sym.front() == '@') {
            return parse(std::move(exp), Tree(std::in_place_index<TokenIdx::Fun>, Box<std::tuple<std::string, Tree, bool>>::make(sym.substr(1), build(std::move(fun), first(std::move(fst))), 1)));
        } else {
            return parse(std::move(exp), std::move(fun), build(std::move(fst), lex(std::move(sym))));
        }
    }

    static Tree lex(std::string_view &&sym) {
        if (sym.front() == '(' && sym.back() == ')') {
            sym.remove_prefix(1);
            sym.remove_suffix(1);
            return parse(std::move(sym));
        } else if (sym.front() == '$') {
            return Tree(std::in_place_index<TokenIdx::Par>, sym.substr(1));
        } else if (sym.size() == 3 && sym == "...") {
            return Tree(std::in_place_index<TokenIdx::Nil>);
        } else if (sym.size() == 1 && sym == "?") {
            return Tree(std::in_place_index<TokenIdx::Chk>);
        } else if (auto const &o = oprs.find(sym.front()); sym.size() == 1 && o != oprs.end()) {
            return Tree(std::in_place_index<TokenIdx::Opr>, *o);
        } else if (auto const &c = cmps.find(sym.front()); sym.size() == 1 && c != cmps.end()) {
            return Tree(std::in_place_index<TokenIdx::Cmp>, *c);
        } else {
            try {
                return Tree(std::in_place_index<TokenIdx::Int>, BigInt::from_string(sym));
            } catch (...) {
                throw std::runtime_error("invalid symbol: " + std::string(sym));
            }
        }
    }

    void calc(size_t stack_depth) {
        static auto const T = 
            Tree(std::in_place_index<TokenIdx::Fun>, Box<std::tuple<std::string, Tree, bool>>::make("T",
                Tree(std::in_place_index<TokenIdx::Fun>, Box<std::tuple<std::string, Tree, bool>>::make("F",
                    Tree(std::in_place_index<TokenIdx::Par>, "T"), 0)), 0));
        static auto const F = 
            Tree(std::in_place_index<TokenIdx::Fun>, Box<std::tuple<std::string, Tree, bool>>::make("T",
                Tree(std::in_place_index<TokenIdx::Fun>, Box<std::tuple<std::string, Tree, bool>>::make("F",
                    Tree(std::in_place_index<TokenIdx::Par>, "F"), 0)), 0));
        if (stack_depth++ >= 65536) {
            throw std::runtime_error("recursion limit exceeded");
        }
    tail_call:
        if (chk_flag()) {
            throw std::runtime_error("keyboard interrupt");
        }
        if (auto papp = std::get_if<TokenIdx::App>(&token)) {
            auto &[fst, snd] = **papp;
            fst.calc(stack_depth);
            if (auto pnil = std::get_if<TokenIdx::Nil>(&fst.token)) {
                token.emplace<TokenIdx::Nil>();
            } else if (auto pchk = std::get_if<TokenIdx::Chk>(&fst.token)) {
                snd.calc(stack_depth);
                *this = snd.token.index() == TokenIdx::Nil ? F : T;
            } else if (auto pfun = std::get_if<TokenIdx::Fun>(&fst.token)) {
                auto &[par, tmp, eager] = **pfun;
                if (eager) {
                    snd.calc(stack_depth);
                }
                tmp.substitute(std::make_shared<std::pair<Tree, bool>>(std::move(snd), 0), std::move(par));
                *this = Tree(std::move(tmp));
                goto tail_call;
            } else if (auto popr = std::get_if<TokenIdx::Opr>(&fst.token)) {
                snd.calc(stack_depth);
                if (auto pint = std::get_if<TokenIdx::Int>(&snd.token); pint && (*pint || popr->first != '/' && popr->first != '%')) {
                    token = std::make_pair(std::move(*popr), std::move(*pint));
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(&snd.token)) {
                    token.emplace<TokenIdx::Nil>();
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto pcmp = std::get_if<TokenIdx::Cmp>(&fst.token)) {
                snd.calc(stack_depth);
                if (auto pint = std::get_if<TokenIdx::Int>(&snd.token)) {
                    token = std::make_pair(std::move(*pcmp), std::move(*pint));
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(&snd.token)) {
                    token.emplace<TokenIdx::Nil>();
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto paoi = std::get_if<TokenIdx::AOI>(&fst.token)) {
                snd.calc(stack_depth);
                if (auto pint = std::get_if<TokenIdx::Int>(&snd.token)) {
                    token = paoi->first.second(std::move(*pint), std::move(paoi->second));
                } else if (auto pnil = std::get_if<TokenIdx::Nil>(&snd.token)) {
                    token.emplace<TokenIdx::Nil>();
                } else {
                    throw std::runtime_error("cannot apply " + fst.translate() + " on: " + snd.translate());
                }
            } else if (auto paci = std::get_if<TokenIdx::ACI>(&fst.token)) {
                snd.calc(stack_depth);
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
                    shr.calc(stack_depth);
                    rec = true;
                }
                *this = shr;
            }
        }
    }

    void substitute(std::shared_ptr<std::pair<Tree, bool>> const &arg, std::string const &tar) {
        if (auto papp = std::get_if<TokenIdx::App>(&token)) {
            auto &[fst, snd] = **papp;
            fst.substitute(arg, tar);
            snd.substitute(arg, tar);
        } else if (auto pfun = std::get_if<TokenIdx::Fun>(&token)) {
            auto &[par, tmp, eager] = **pfun;
            if (par != tar) {
                tmp.substitute(arg, tar);
            }
        } else if (auto ppar = std::get_if<TokenIdx::Par>(&token)) {
            if (*ppar == tar) {
                token.emplace<TokenIdx::Arg>(arg);
            }
        }
    }

    void analyze(std::unordered_set<std::string> &set) {
        if (auto papp = std::get_if<TokenIdx::App>(&token)) {
            auto &[fst, snd] = **papp;
            fst.analyze(set);
            snd.analyze(set);
        } else if (auto pfun = std::get_if<TokenIdx::Fun>(&token)) {
            auto &[par, tmp, eager] = **pfun;
            if (set.find(par) != set.end()) {
                tmp.analyze(set);
            } else {
                auto pos = set.insert(par).first;
                tmp.analyze(set);
                set.erase(pos);
            }
        } else if (auto ppar = std::get_if<TokenIdx::Par>(&token)) {
            if (set.find(*ppar) == set.end()) {
                if (auto const &itr = map.find(*ppar); itr != map.end()) {
                    token.emplace<TokenIdx::Arg>(itr->second);
                } else {
                    throw std::runtime_error("unbound variable: $" + *ppar);
                }
            }
        }
    }

    static inline std::unordered_map<std::string, std::shared_ptr<std::pair<Tree, bool>>> map;

public:
    Tree(Tree const &other)
        : token(other.token) {}

    Tree &operator=(Tree const &other) {
        token = other.token;
        return *this;
    }

    Tree(Tree &&other)
        : token(std::exchange(other.token, std::nullopt)) {}

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
            } else if (auto parg = std::get_if<TokenIdx::Fun>(&token)) {
                auto &[par, tmp, eager] = **parg;
                flat.push(std::exchange(tmp.token, std::nullopt));
            } else if (auto parg = std::get_if<TokenIdx::Arg>(&token)) {
                auto &shr = (*parg)->first;
                if (parg->use_count() == 1) {
                    flat.push(std::exchange(shr.token, std::nullopt));
                }
            }
        }
    }

    static auto const &put(std::string_view &&exp, std::string &&par, bool calc) {
        auto res = parse(std::move(exp));
        std::unordered_set<std::string> set;
        res.analyze(set);
        map.erase(par);
        if (calc) {
            clr_flag();
            res.calc(0);
        }
        return map.emplace(std::move(par), std::make_shared<std::pair<Tree, bool>>(std::move(res), calc)).first->second->first;
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
        } else if (auto pfun = std::get_if<TokenIdx::Fun>(&token)) {
            auto &[par, tmp, eager] = **pfun;
            auto s = (eager ? "^" : "\\") + par + " " + tmp.translate(0, rb && !rb);
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
        } else {
            assert(false);  // unreachable
        }
    }
};

#define PS_IN ">> "
#define PS_OUT "=> "
#define PS_RES "== "

int main(int argc, char *argv[]) {
    StreamStatus status = io_check();
    for (bool end = false; not end;) {
        if (status.stderr_flag && status.stdin_flag) {
            std::cerr << PS_IN;
        }
        std::string buf;
        std::getline(std::cin, buf);
        std::string_view exp = buf;
        if (std::cin.eof()) {
            end = true;
            if (status.stderr_flag && status.stdin_flag) {
                std::cerr << std::endl;
            }
        }
        try {
            if (auto cmd = read(exp); cmd.empty() || cmd.size() == 1 && cmd == "#") {
                continue;
            } else if (cmd.front() == ':') {
                Tree::put(std::move(exp), std::string(cmd.substr(1)), 0);
            } else if (cmd.size() == 3 && cmd == "cal") {
                auto &res = Tree::put(std::move(exp), "", 1);
                if (status.stderr_flag && status.stdout_flag) {
                    std::cerr << PS_RES;
                }
                std::cout << res.translate() << std::endl;
            } else if (cmd.size() == 3 && cmd == "dir") {
                for (auto &[par, arg] : Tree::dir()) {
                    if (status.stderr_flag && status.stdout_flag) {
                        std::cerr << PS_OUT;
                    }
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

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
#define DEF 0
#define SET 1
#ifndef STACK_MAX
#define STACK_MAX 8388608 // 8 MiB
#endif
int *stack_top;
bool stack_err() {
    int dummy;
    return (char *)stack_top - (char *)&dummy >= STACK_MAX / 2;
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
class Tree {
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
    friend Node<std::pair<std::string, Tree>>;
    friend Node<std::pair<Tree, Tree>>;
    enum Token: std::size_t {
        Ini, Int,
        Opr, OprInt,
        Cmp, CmpInt,
        App, Fun,
        Par, Arg,
        Def, Set,
    };
    using Var = std::variant<
        std::monostate, StrInt,
        std::pair<char, opr_t>, std::pair<std::pair<char, opr_t>, StrInt>,
        std::pair<char, cmp_t>, std::pair<std::pair<char, cmp_t>, StrInt>,
        Node<std::pair<Tree, Tree>>, Node<std::pair<std::string, Tree>>,
        std::string, std::shared_ptr<std::pair<Tree, bool>>,
        std::string, std::string>;
    Var var;
    template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<Var, Args &&...>>>
    Tree(Args &&...args): var(std::forward<Args>(args)...) {}
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
    static Tree parse(Slice &&exp, Tree &&fst = std::monostate()) {
        if (auto sym = read(exp); sym.empty()) {
            return first(std::move(fst));
        } else if (sym[0] == '\\') {
            return build(std::move(fst), Node<std::pair<std::string, Tree>>::make(sym(1, 0), parse(std::move(exp))));
        } else if (sym[0] == '|') {
            return parse(std::move(exp), Node<std::pair<std::string, Tree>>::make(sym(1, 0), first(std::move(fst))));
        } else {
            return parse(std::move(exp), build(std::move(fst), lex(std::move(sym))));
        }
    }
    static Tree lex(Slice const &sym) {
        if (sym[0] == '(' && sym[-1] == ')') {
            return parse(sym(1, -1));
        } else if (sym[0] == '$') {
            return Tree(std::in_place_index<Token::Par>, sym(1, 0));
        } else if (sym[0] == '&') {
            return Tree(std::in_place_index<Token::Def>, sym(1, 0));
        } else if (sym[0] == '!') {
            return Tree(std::in_place_index<Token::Set>, sym(1, 0));
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
    void calculate() {
        if (stack_err()) {
            throw std::runtime_error("recursion too deep");
        }
        switch (var.index()) {
        case Token::App: {
            auto &fst = std::get<Token::App>(var)->first;
            auto &snd = std::get<Token::App>(var)->second;
            if (fst.calculate(), fst.var.index() == Token::Fun) {
                auto tmp = std::move(std::get<Token::Fun>(fst.var)->second);
                tmp.substitute(std::make_shared<std::pair<Tree, bool>>(std::move(snd), false), std::move(std::get<Token::Fun>(fst.var)->first));
                tmp.calculate();
                *this = std::move(tmp);
            } else if (fst.var.index() == Token::Opr && (snd.calculate(), snd.var.index() == Token::Int) &&
                (std::get<Token::Opr>(fst.var).first != '/' && std::get<Token::Opr>(fst.var).first != '%' || std::get<Token::Int>(snd.var))) {
                var = std::make_pair(std::get<Token::Opr>(fst.var), std::move(std::get<Token::Int>(snd.var)));
            } else if (fst.var.index() == Token::Cmp && (snd.calculate(), snd.var.index() == Token::Int)) {
                var = std::make_pair(std::get<Token::Cmp>(fst.var), std::move(std::get<Token::Int>(snd.var)));
            } else if (fst.var.index() == Token::OprInt && (snd.calculate(), snd.var.index() == Token::Int)) {
                var = std::get<Token::OprInt>(fst.var).first.second(std::move(std::get<Token::Int>(snd.var)), std::move(std::get<Token::OprInt>(fst.var).second));
            } else if (fst.var.index() == Token::CmpInt && (snd.calculate(), snd.var.index() == Token::Int)) {
                var = std::get<Token::CmpInt>(fst.var).first.second(std::move(std::get<Token::Int>(snd.var)), std::move(std::get<Token::CmpInt>(fst.var).second))
                    ? Node<std::pair<std::string, Tree>>::make("T", Node<std::pair<std::string, Tree>>::make("F", Tree(std::in_place_index<Token::Par>, "T")))
                    : Node<std::pair<std::string, Tree>>::make("T", Node<std::pair<std::string, Tree>>::make("F", Tree(std::in_place_index<Token::Par>, "F")));
            } else {
                throw std::runtime_error("invalid application: " + std::get<0>(fst.translate()) + " on " + std::get<0>(snd.translate()));
            }
        } break;
        case Token::Arg: {
            auto arg = std::move(std::get<Token::Arg>(var));
            if (not arg->second) {
                arg->first.calculate();
                arg->second = true;
            }
            *this = arg.use_count() == 1 ? std::move(arg->first) : arg->first;
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
    template <bool Spc>
    static inline std::unordered_map<std::string, Tree> dct;
public:
    template <bool Spc>
    static auto const &put(Slice &&exp, std::string const &key) {
        auto val = parse(std::move(exp));
        if constexpr (Spc == SET) {
            val.calculate();
        }
        return dct<Spc>.insert_or_assign(key, std::move(val)).first->second;
    }
    template <bool Spc>
    static auto const &dir() {
        return dct<Spc>;
    }
    template <bool Spc>
    static void clr() {
        return dct<Spc>.clear();
    }
    std::tuple<std::string, bool, bool> translate() const {
        switch (var.index()) {
        case Token::Fun:
            return {"\\" + std::get<Token::Fun>(var)->first + " " + std::get<0>(std::get<Token::Fun>(var)->second.translate()), 1, 0};
        case Token::Par:
            return {"$" + std::get<Token::Par>(var), 0, 0};
        case Token::Def:
            return {"&" + std::get<Token::Def>(var), 0, 0};
        case Token::Set:
            return {"!" + std::get<Token::Set>(var), 0, 0};
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
        } break;
        case Token::Arg:
            return std::get<Token::Arg>(var)->first.translate();
        default:
            assert(false); // unreachable
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
    struct rlimit rlim;
    getrlimit(RLIMIT_STACK, &rlim);
    rlim.rlim_cur = STACK_MAX;
    setrlimit(RLIMIT_STACK, &rlim);
#endif
    std::string ps_in = check_stderr && check_stdin ? ">> " : "";
    std::string ps_out = check_stderr && check_stdout ? "=> " : "";
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
                Tree::put<DEF>(std::move(exp), cmd(1, 0));
            } else if (cmd[0] == '!') {
                Tree::put<SET>(std::move(exp), cmd(1, 0));
            } else if (cmd.size() == 3 && cmd == "fmt") {
                auto &res = Tree::put<DEF>(std::move(exp), "");
                std::cerr << ps_out;
                std::cout << std::get<0>(res.translate()) << std::endl;
            } else if (cmd.size() == 3 && cmd == "cal") {
                auto &res = Tree::put<SET>(std::move(exp), "");
                std::cerr << ps_out;
                std::cout << std::get<0>(res.translate()) << std::endl;
            } else if (cmd.size() == 3 && cmd == "dir") {
                for (auto const &[key, val] : Tree::dir<DEF>()) {
                    std::cerr << ps_out;
                    std::cout << std::left << std::setw(10) << "&" + (key.size() <= 8 ? key : key.substr(0, 6) + "..") << std::get<0>(val.translate()) << std::endl;
                }
                for (auto const &[key, val] : Tree::dir<SET>()) {
                    std::cerr << ps_out;
                    std::cout << std::left << std::setw(10) << "!" + (key.size() <= 8 ? key : key.substr(0, 6) + "..") << std::get<0>(val.translate()) << std::endl;
                }
            } else if (cmd.size() == 3 && cmd == "clr") {
                Tree::clr<DEF>();
                Tree::clr<SET>();
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

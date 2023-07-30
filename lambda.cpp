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
#define SET 1
#define DEF 0
#define STACK_MAX 8388608
#define STACK_ERR 4194304
inline int *stack_top;
inline bool stack_err(int dummy = 0) {
    return (char *)stack_top - (char *)&dummy >= STACK_ERR;
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
    template <bool SPC>
    static inline std::unordered_map<std::string, Tree const> map;
    using VarT = std::variant<std::string, StrInt, char, std::pair<char, StrInt>, Node<std::pair<Tree, Tree>>, Node<std::pair<std::string, Tree>>, std::shared_ptr<std::pair<Tree, bool>>>;
    char tid;
    VarT var;
    template <typename T>
    Tree(char tid, T &&var): tid(tid), var(static_cast<T &&>(var)) {}
public:
    template <bool SPC>
    static auto const &put(std::string const &key, Tree &&val) {
        if constexpr (SPC == SET) {
            val.calculate();
        }
        if (auto const &it = map<SPC>.find(key); it != map<SPC>.end()) {
            map<SPC>.erase(it);
        }
        return map<SPC>.emplace(key, std::move(val)).first->second;
    }
    template <bool SPC>
    static auto const &dir() {
        return map<SPC>;
    }
    template <bool SPC>
    static void clr() {
        return map<SPC>.clear();
    }
    static Tree parse(std::string &&exp) {
        std::string sym;
        if (!(exp >> sym)) {
            throw std::runtime_error("empty expression");
        }
        auto build = [&](Tree &&snd) { return std::move(snd); };
        return sym.back() == ':'
            ? build(Tree(':', Node<std::pair<std::string, Tree>>::make(sym.substr(0, sym.size() - 1), parse(std::move(exp)))))
            : parse(std::move(exp), build(lex(std::move(sym))));
    }
    static Tree parse(std::string &&exp, Tree &&fst) {
        std::string sym;
        if (!(exp >> sym)) {
            return std::move(fst);
        }
        auto build = [&](Tree &&snd) { return Tree('|', Node<std::pair<Tree, Tree>>::make(std::move(fst), std::move(snd))); };
        return sym.back() == ':'
            ? build(Tree(':', Node<std::pair<std::string, Tree>>::make(sym.substr(0, sym.size() - 1), parse(std::move(exp)))))
            : parse(std::move(exp), build(lex(std::move(sym))));
    }
    static Tree lex(std::string &&sym) {
        if (sym.front() == '(' && sym.back() == ')') {
            return parse(sym.substr(1, sym.size() - 2));
        } else if (sym.front() == '$') {
            return Tree('$', sym.substr(1));
        } else if (sym.front() == '&') {
            return Tree('&', sym.substr(1));
        } else if (sym.front() == '!') {
            return Tree('!', sym.substr(1));
        } else if (auto const &o = oprs.find(sym[0]); sym.size() == 1 && o != oprs.end()) {
            return Tree('o', o->first);
        } else if (auto const &c = cmps.find(sym[0]); sym.size() == 1 && c != cmps.end()) {
            return Tree('c', c->first);
        } else try {
            return Tree('i', StrInt::from_string(sym));
        } catch (...) {
            return Tree('s', std::move(sym));
        }
    }
    void calculate() {
        if (stack_err()) {
            throw std::runtime_error("recursion too deep");
        }
        switch (tid) {
        case '|': {
            auto &fst = std::get<Node<std::pair<Tree, Tree>>>(var)->first;
            auto &snd = std::get<Node<std::pair<Tree, Tree>>>(var)->second;
            if (fst.calculate(), fst.tid == ':') {
                auto nod = std::move(std::get<Node<std::pair<std::string, Tree>>>(fst.var));
                nod->second.substitute(std::make_shared<std::pair<Tree, bool>>(std::move(snd), false), nod->first);
                *this = std::move(nod->second);
                calculate();
            } else if (snd.calculate(), snd.tid == 'i') {
                switch (fst.tid) {
                case 'o':
                    tid = 'O';
                    var = std::pair<char, StrInt>{std::get<char>(fst.var), std::get<StrInt>(snd.var)};
                    break;
                case 'c':
                    tid = 'C';
                    var = std::pair<char, StrInt>{std::get<char>(fst.var), std::get<StrInt>(snd.var)};
                    break;
                case 'O':
                    tid = 'i';
                    var = oprs.at(std::get<std::pair<char, StrInt>>(fst.var).first)(std::get<StrInt>(snd.var), std::get<std::pair<char, StrInt>>(fst.var).second);
                    break;
                case 'C':
                    tid = ':';
                    var = cmps.at(std::get<std::pair<char, StrInt>>(fst.var).first)(std::get<StrInt>(snd.var), std::get<std::pair<char, StrInt>>(fst.var).second)
                        ? Node<std::pair<std::string, Tree>>::make("T", Tree(':', Node<std::pair<std::string, Tree>>::make("F", Tree('$', "T"))))
                        : Node<std::pair<std::string, Tree>>::make("T", Tree(':', Node<std::pair<std::string, Tree>>::make("F", Tree('$', "F"))));
                    break;
                }
            }
        } break;
        case '#': {
            auto tmp = std::get<std::shared_ptr<std::pair<Tree, bool>>>(var);
            if (not tmp->second) {
                tmp->first.calculate();
                tmp->second = true;
            }
            *this = tmp->first;
        } break;
        case '!':
            if (auto const &it = map<SET>.find(std::get<std::string>(var)); it != map<SET>.end()) {
                *this = it->second.
                deep_copy(); break;
            }
            throw std::runtime_error("undefined symbol: !" + std::get<std::string>(var));
        case '&':
            if (auto const &it = map<DEF>.find(std::get<std::string>(var)); it != map<DEF>.end()) {
                *this = it->second;
                calculate(); break;
            }
            throw std::runtime_error("undefined symbol: &" + std::get<std::string>(var));
        case '$':
            throw std::runtime_error("unbound parameter: $" + std::get<std::string>(var));
        }
    }
    void substitute(std::shared_ptr<std::pair<Tree, bool>> const &ptr, std::string const &par) {
        switch (tid) {
        case '|':
            std::get<Node<std::pair<Tree, Tree>>>(var)->first.substitute(ptr, par);
            std::get<Node<std::pair<Tree, Tree>>>(var)->second.substitute(ptr, par);
            break;
        case ':':
            if (std::get<Node<std::pair<std::string, Tree>>>(var)->first != par) {
                std::get<Node<std::pair<std::string, Tree>>>(var)->second.substitute(ptr, par);
            }
            break;
        case '$':
            if (std::get<std::string>(var) == par) {
                tid = '#';
                var = ptr;
            }
            break;
        }
    }
    Tree deep_copy() const {
        switch (tid) {
        case '|':
            return Tree('|', Node<std::pair<Tree, Tree>>::make(
                std::get<Node<std::pair<Tree, Tree>>>(var)->first.deep_copy(),
                std::get<Node<std::pair<Tree, Tree>>>(var)->second.deep_copy()));
        case ':':
            return Tree(':', Node<std::pair<std::string, Tree>>::make(
                std::get<Node<std::pair<std::string, Tree>>>(var)->first,
                std::get<Node<std::pair<std::string, Tree>>>(var)->second.deep_copy()));
        case '#':
            return Tree('#', std::make_shared<std::pair<Tree, bool>>(
                std::get<std::shared_ptr<std::pair<Tree, bool>>>(var)->first.deep_copy(),
                std::get<std::shared_ptr<std::pair<Tree, bool>>>(var)->second));
        default:
            return *this;
        }
    }
    std::pair<std::string, std::pair<bool, bool>> translate() const {
        switch (tid) {
        case ':':
            return {std::get<Node<std::pair<std::string, Tree>>>(var)->first + ": " + std::get<Node<std::pair<std::string, Tree>>>(var)->second.translate().first, {1, 0}};
        case '$':
            return {"$" + std::get<std::string>(var), {0, 0}};
        case '&':
            return {"&" + std::get<std::string>(var), {0, 0}};
        case '!':
            return {"!" + std::get<std::string>(var), {0, 0}};
        case '#':
            return std::get<std::shared_ptr<std::pair<Tree, bool>>>(var)->first.translate();
        case 's':
            return {std::get<std::string>(var), {0, 0}};
        case 'i':
            return {std::get<StrInt>(var).to_string(), {0, 0}};
        case 'o':
            return {std::string{std::get<char>(var)}, {0, 0}};
        case 'c':
            return {std::string{std::get<char>(var)}, {0, 0}};
        case 'O':
            return {std::string{std::get<std::pair<char, StrInt>>(var).first, ' '} + std::get<std::pair<char, StrInt>>(var).second.to_string(), {0, 1}};
        case 'C':
            return {std::string{std::get<std::pair<char, StrInt>>(var).first, ' '} + std::get<std::pair<char, StrInt>>(var).second.to_string(), {0, 1}};
        case '|':
            auto fst = std::get<Node<std::pair<Tree, Tree>>>(var)->first.translate();
            auto snd = std::get<Node<std::pair<Tree, Tree>>>(var)->second.translate();
            return {(fst.second.first ? "(" + fst.first + ")" : fst.first) + " " + (snd.second.second ? "(" + snd.first + ")" : snd.first), {snd.second.first && !snd.second.second, 1}};
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
    for (bool end = false; !end;) {
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
            if (!(exp >> buf) || buf == "#") {
                continue;
            } else if (buf == "set") {
                if (exp >> buf) {
                    Tree::put<SET>(buf, Tree::parse(std::move(exp)));
                } else {
                    throw std::runtime_error("missing symbol");
                }
            } else if (buf == "def") {
                if (exp >> buf) {
                    Tree::put<DEF>(buf, Tree::parse(std::move(exp)));
                } else {
                    throw std::runtime_error("missing symbol");
                }
            } else if (buf == "cal") {
                auto &res = Tree::put<SET>("", Tree::parse(std::move(exp)));
                std::cerr << ps_out;
                std::cout << res.translate().first << std::endl;
            } else if (buf == "fmt") {
                auto &res = Tree::put<DEF>("", Tree::parse(std::move(exp)));
                std::cerr << ps_out;
                std::cout << res.translate().first << std::endl;
            } else if (buf == "dir") {
                for (auto const &l : Tree::dir<SET>()) {
                    std::cerr << ps_out;
                    std::cout << std::left << std::setw(10) << '!' + (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second.translate().first << std::endl;
                }
                for (auto const &l : Tree::dir<DEF>()) {
                    std::cerr << ps_out;
                    std::cout << std::left << std::setw(10) << '&' + (l.first.size() <= 8 ? l.first : l.first.substr(0, 6) + "..") << l.second.translate().first << std::endl;
                }
            } else if (buf == "clr") {
                Tree::clr<SET>();
                Tree::clr<DEF>();
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

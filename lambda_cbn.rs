#![feature(box_patterns)]
#![feature(if_let_guard)]
use std::arch::asm;
use std::cell::RefCell;
use std::collections::HashMap;
use std::io;
use std::mem;
use std::rc::Rc;
extern crate atty;
extern crate lazy_static;
extern crate num_bigint;
extern crate num_traits;
use atty::Stream;
use lazy_static::lazy_static;
use num_bigint::BigInt;
use num_traits::Zero;
lazy_static! {
    static ref PS_IN: &'static str = if atty::is(Stream::Stderr) && atty::is(Stream::Stdin) { ">> " } else { "" };
    static ref PS_OUT: &'static str = if atty::is(Stream::Stderr) && atty::is(Stream::Stdout) { "=> " } else { "" };
    static ref PS_RES: &'static str = if atty::is(Stream::Stderr) && atty::is(Stream::Stdout) { "== " } else { "" };
    static ref ADD_LN: bool = if atty::is(Stream::Stderr) && atty::is(Stream::Stdin) { true } else { false };
    static ref STACK_MAX: usize = 0o4000_0000;
    static ref STACK_TOP: usize = stack_ptr();
}
fn stack_ptr() -> usize {
    unsafe {
        let mut ptr: usize;
        asm!("mov {}, rsp", out(reg) ptr);
        ptr
    }
}
fn stack_err() -> bool {
    *STACK_TOP - *STACK_MAX / 2 > stack_ptr()
}
fn read(exp: &str) -> Result<(&str, &str), String> {
    let i = exp.as_bytes().iter().position(|&c| c != b' ').unwrap_or(exp.len());
    let mut ctr: usize = 0;
    for (j, &c) in exp.as_bytes().iter().enumerate().skip(i) {
        if ctr == 0 && c == b' ' {
            return Ok((&exp[i..j], &exp[j..]));
        } else if c == b'(' {
            ctr += 1;
        } else if c == b')' {
            ctr -= 1;
        }
    }
    if ctr != 0 {
        Err("unmatched parenthesis".to_string())
    } else {
        Ok((&exp[i..], ""))
    }
}
type Oprt = fn(BigInt, BigInt) -> BigInt;
type Cmpt = fn(BigInt, BigInt) -> bool;
static OPRS: [(&str, Oprt); 5] = [
    ("+", |a, b| a + b),
    ("-", |a, b| a - b),
    ("*", |a, b| a * b),
    ("/", |a, b| a / b),
    ("%", |a, b| a % b),
];
static CMPS: [(&str, Cmpt); 3] = [
    (">", |a, b| a > b),
    ("<", |a, b| a < b),
    ("=", |a, b| a == b),
];
#[derive(Clone)]
enum Tree {
    Empty,
    Ellipsis,
    Int(BigInt),
    Opr(Oprt, String),
    Cmp(Cmpt, String),
    OprInt(Oprt, String, BigInt),
    CmpInt(Cmpt, String, BigInt),
    Fun(String, Box<Tree>),
    Out(String, Box<Tree>),
    App(Box<Tree>, Box<Tree>),
    Arg(Rc<RefCell<(Tree, bool)>>),
    Par(String),
}
impl Tree {
    fn first(fst: Tree) -> Result<Tree, String> {
        match fst {
            Tree::Empty => Err("empty expression".to_string()),
            _ => Ok(fst),
        }
    }
    fn build(fst: Tree, snd: Tree) -> Result<Tree, String> {
        match fst {
            Tree::Empty => Ok(snd),
            _ => Ok(Tree::App(Box::new(fst), Box::new(snd))),
        }
    }
    fn parse(exp: &str, fun: Tree, fst: Tree) -> Result<Tree, String> {
        let (sym, rest) = read(exp)?;
        if sym.is_empty() {
            Self::build(fun, Self::first(fst)?)
        } else if sym.starts_with('\\') {
            let tmp = Tree::Fun(sym[1..].to_string(), Box::new(Self::parse(rest, Tree::Empty, Tree::Empty)?));
            Self::build(fun, Self::build(fst, tmp)?)
        } else if sym.starts_with('|') {
            let tmp = Tree::Fun(sym[1..].to_string(), Box::new(Self::build(fun, Self::first(fst)?)?));
            Self::parse(rest, tmp, Tree::Empty)
        } else if sym.starts_with('^') {
            let tmp = Tree::Out(sym[1..].to_string(), Box::new(Self::parse(rest, Tree::Empty, Tree::Empty)?));
            Self::build(fun, Self::build(fst, tmp)?)
        } else if sym.starts_with('@') {
            let tmp = Tree::Out(sym[1..].to_string(), Box::new(Self::build(fun, Self::first(fst)?)?));
            Self::parse(rest, tmp, Tree::Empty)
        } else {
            Self::parse(rest, fun, Self::build(fst, Self::lex(sym)?)?)
        }
    }
    fn lex(sym: &str) -> Result<Tree, String> {
        if sym.starts_with('(') && sym.ends_with(')') {
            Self::parse(&sym[1..sym.len() - 1], Tree::Empty, Tree::Empty)
        } else if sym.starts_with('$') {
            Ok(Tree::Par(sym[1..].to_string()))
        } else if sym == "..." {
            Ok(Tree::Ellipsis)
        } else if let Some((name, opr)) = OPRS.iter().find(|(s, _)| *s == sym) {
            Ok(Tree::Opr(*opr, name.to_string()))
        } else if let Some((name, cmp)) = CMPS.iter().find(|(s, _)| *s == sym) {
            Ok(Tree::Cmp(*cmp, name.to_string()))
        } else if let Ok(num) = sym.parse::<BigInt>() {
            Ok(Tree::Int(num))
        } else {
            Err(format!("unknown symbol: {}", sym))
        }
    }
    fn calculate(self) -> Result<Self, String> {
        if stack_err() {
            return Err("recursion limit exceeded".to_string());
        }
        match self {
            Tree::App(box fst, box snd) => match fst.calculate()? {
                Tree::Ellipsis => Ok(Tree::Ellipsis),
                Tree::Fun(sym, box mut tmp) => {
                    tmp.substitute(&Rc::new(RefCell::new((snd, false))), &sym);
                    tmp.calculate()
                }
                Tree::Out(sym, box mut tmp) => {
                    let snd = snd.calculate()?;
                    tmp.substitute(&Rc::new(RefCell::new((snd, true))), &sym);
                    tmp.calculate()
                }
                Tree::Opr(opr, name) => match snd.calculate()? {
                    Tree::Int(int) if !int.is_zero() || (name != "/" && name != "%") => Ok(Tree::OprInt(opr, name, int)),
                    snd => Err(format!("cannot apply {} on: {}", name, snd.translate(false, false))),
                },
                Tree::Cmp(cmp, name) => match snd.calculate()? {
                    Tree::Int(int) => Ok(Tree::CmpInt(cmp, name, int)),
                    snd => Err(format!("cannot apply {} on: {}", name, snd.translate(false, false))),
                },
                Tree::OprInt(opr, name, int) => match snd.calculate()? {
                    Tree::Int(val) => Ok(Tree::Int(opr(val, int))),
                    snd => Err(format!("cannot apply {} {} on: {}", name, int, snd.translate(false, false))),
                },
                Tree::CmpInt(cmp, name, int) => match snd.calculate()? {
                    Tree::Int(val) => Ok(Tree::Fun("T".to_string(), Box::new(Tree::Fun("F".to_string(), Box::new(Tree::Par(if cmp(val, int) { "T" } else { "F" }.to_string())))))),
                    snd => Err(format!("cannot apply {} {} on: {}", name, int, snd.translate(false, false))),
                },
                fst => Err(format!("invalid function: {}", fst.translate(false, false))),
            },
            Tree::Arg(arg) => {
                let (shr, rec) = &mut *arg.borrow_mut();
                if !*rec {
                    *shr = mem::replace(shr, Tree::Empty).calculate()?;
                    *rec = true;
                }
                Ok(if Rc::strong_count(&arg) == 1 { mem::replace(shr, Tree::Empty) } else { shr.clone() })
            }
            Tree::Par(key) => {
                Err(format!("unbound variable: ${}", key))
            }
            _ => Ok(self),
        }
    }
    fn substitute(&mut self, arg: &Rc<RefCell<(Tree, bool)>>, par: &String) {
        match self {
            Tree::Fun(sym, box tmp) if sym != par => {
                tmp.substitute(arg, par);
            }
            Tree::Out(sym, box tmp) if sym != par => {
                tmp.substitute(arg, par);
            }
            Tree::App(box fst, box snd) => {
                fst.substitute(arg, par);
                snd.substitute(arg, par);
            }
            Tree::Par(sym) if sym == par => {
                *self = Tree::Arg(arg.clone());
            }
            _ => {}
        }
    }
    fn translate(&self, lb: bool, rb: bool) -> String {
        match self {
            Tree::Fun(sym, box tree) => {
                let xxx = format!("\\{} {}", sym, tree.translate(false, rb && !rb));
                if rb { format!("({})", xxx) } else { xxx }
            }
            Tree::Out(sym, box tree) => {
                let xxx = format!("^{} {}", sym, tree.translate(false, rb && !rb));
                if rb { format!("({})", xxx) } else { xxx }
            }
            Tree::Ellipsis => "...".to_string(),
            Tree::Int(i) => i.to_string(),
            Tree::Opr(_, name) => name.clone(),
            Tree::Cmp(_, name) => name.clone(),
            Tree::OprInt(_, name, int) => {
                let xxx = format!("{} {}", name, int);
                if lb { format!("({})", xxx) } else { xxx }
            }
            Tree::CmpInt(_, name, int) => {
                let xxx = format!("{} {}", name, int);
                if lb { format!("({})", xxx) } else { xxx }
            }
            Tree::App(box fst, box snd) => {
                let xxx = format!("{} {}", fst.translate(lb && !lb, true), snd.translate(true, rb && !lb));
                if lb { format!("({})", xxx) } else { xxx }
            }
            Tree::Arg(arg) => {
                let (shr, rec) = &*arg.borrow();
                shr.translate(lb, rb)
            }
            Tree::Par(key) => format!("${}", key),
            _ => panic!("invalid tree"),
        }
    }
}
fn process(input: String, map: &mut HashMap<String, Rc<RefCell<(Tree, bool)>>>) -> Result<(), String> {
    let (cmd, exp) = read(&input)?;
    if cmd.is_empty() || cmd == "#" {
        Ok(())
    } else if cmd.starts_with(':') {
        let mut res = Tree::parse(exp, Tree::Empty, Tree::Empty)?;
        for (key, val) in map.iter() {
            res.substitute(val, key);
        }
        map.insert(cmd[1..].to_string(), Rc::new(RefCell::new((res, false))));
        Ok(())
    } else if cmd == "cal" {
        let mut res = Tree::parse(exp, Tree::Empty, Tree::Empty)?;
        for (key, val) in map.iter() {
            res.substitute(val, key);
        }
        let res = res.calculate()?;
        eprint!("{}", *PS_RES);
        println!("{}", res.translate(false, false));
        map.insert("".to_string(), Rc::new(RefCell::new((res, true))));
        Ok(())
    } else if cmd == "dir" {
        for (key, val) in map.iter() {
            eprint!("{}", *PS_OUT);
            println!(":{}", key);
        }
        Ok(())
    } else if cmd == "clr" {
        map.clear();
        Ok(())
    } else {
        Err(format!("unknown command: {}", cmd))
    }
}
fn main() {
    let mut map = HashMap::new();
    let mut nxt = true;
    while nxt {
        eprint!("{}", *PS_IN);
        let mut input = String::new();
        io::stdin().read_line(&mut input).unwrap();
        if input.ends_with('\r') {
            input.pop();
        }
        if input.ends_with('\n') {
            input.pop();
        } else if *ADD_LN {
            eprintln!();
            nxt = false;
        } else {
            nxt = false;
        }
        if input.ends_with('\r') {
            input.pop();
        }
        if let Err(err) = process(input, &mut map) {
            eprintln!("Runtime Error: {}", err);
        }
    }
}

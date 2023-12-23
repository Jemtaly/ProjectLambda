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
    Undefined,
    Ellipsis,
    Int(BigInt),
    Opr(Oprt, String),
    Cmp(Cmpt, String),
    AOI(Oprt, String, BigInt),
    ACI(Cmpt, String, BigInt),
    LEF(String, Box<Tree>), // Lazy Evaluation Function
    EEF(String, Box<Tree>), // Eager Evaluation Function
    App(Box<Tree>, Box<Tree>),
    Arg(Rc<RefCell<(Tree, bool)>>),
    Par(String),
}
impl Tree {
    fn first(fst: Tree) -> Result<Tree, String> {
        match fst {
            Tree::Undefined => Err("Undefined expression".to_string()),
            _ => Ok(fst),
        }
    }
    fn build(fst: Tree, snd: Tree) -> Result<Tree, String> {
        match fst {
            Tree::Undefined => Ok(snd),
            _ => Ok(Tree::App(Box::new(fst), Box::new(snd))),
        }
    }
    fn parse(exp: &str, fun: Tree, fst: Tree) -> Result<Tree, String> {
        let (sym, rest) = read(exp)?;
        if sym.is_empty() {
            Self::build(fun, Self::first(fst)?)
        } else if sym.starts_with('\\') {
            let tmp = Tree::LEF(sym[1..].to_string(), Box::new(Self::parse(rest, Tree::Undefined, Tree::Undefined)?));
            Self::build(fun, Self::build(fst, tmp)?)
        } else if sym.starts_with('|') {
            let tmp = Tree::LEF(sym[1..].to_string(), Box::new(Self::build(fun, Self::first(fst)?)?));
            Self::parse(rest, tmp, Tree::Undefined)
        } else if sym.starts_with('^') {
            let tmp = Tree::EEF(sym[1..].to_string(), Box::new(Self::parse(rest, Tree::Undefined, Tree::Undefined)?));
            Self::build(fun, Self::build(fst, tmp)?)
        } else if sym.starts_with('@') {
            let tmp = Tree::EEF(sym[1..].to_string(), Box::new(Self::build(fun, Self::first(fst)?)?));
            Self::parse(rest, tmp, Tree::Undefined)
        } else {
            Self::parse(rest, fun, Self::build(fst, Self::lex(sym)?)?)
        }
    }
    fn lex(sym: &str) -> Result<Tree, String> {
        if sym.starts_with('(') && sym.ends_with(')') {
            Self::parse(&sym[1..sym.len() - 1], Tree::Undefined, Tree::Undefined)
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
                Tree::LEF(sym, box mut tmp) => {
                    tmp.replace(&Rc::new(RefCell::new((snd, false))), &sym);
                    tmp.calculate()
                }
                Tree::EEF(sym, box mut tmp) => {
                    let snd = snd.calculate()?;
                    tmp.replace(&Rc::new(RefCell::new((snd, true))), &sym);
                    tmp.calculate()
                }
                Tree::Opr(opr, name) => match snd.calculate()? {
                    Tree::Int(int) if !int.is_zero() || (name != "/" && name != "%") => Ok(Tree::AOI(opr, name, int)),
                    snd => Err(format!("cannot apply {} on: {}", name, snd.translate(false, false))),
                },
                Tree::Cmp(cmp, name) => match snd.calculate()? {
                    Tree::Int(int) => Ok(Tree::ACI(cmp, name, int)),
                    snd => Err(format!("cannot apply {} on: {}", name, snd.translate(false, false))),
                },
                Tree::AOI(opr, name, int) => match snd.calculate()? {
                    Tree::Int(val) => Ok(Tree::Int(opr(val, int))),
                    snd => Err(format!("cannot apply {} {} on: {}", name, int, snd.translate(false, false))),
                },
                Tree::ACI(cmp, name, int) => match snd.calculate()? {
                    Tree::Int(val) => Ok(Tree::LEF("T".to_string(), Box::new(Tree::LEF("F".to_string(), Box::new(Tree::Par(if cmp(val, int) { "T" } else { "F" }.to_string())))))),
                    snd => Err(format!("cannot apply {} {} on: {}", name, int, snd.translate(false, false))),
                },
                fst => Err(format!("invalid function: {}", fst.translate(false, false))),
            },
            Tree::Arg(arg) => {
                let (shr, rec) = &mut *arg.borrow_mut();
                if Rc::strong_count(&arg) == 1 {
                    if !*rec { mem::replace(shr, Tree::Undefined).calculate() } else { Ok(mem::replace(shr, Tree::Undefined)) }
                } else {
                    if !*rec {
                        *shr = mem::replace(shr, Tree::Undefined).calculate()?;
                        *rec = true;
                    }
                    Ok(shr.clone())
                }
            }
            Tree::Par(par) => {
                Err(format!("unbound variable: ${}", par))
            }
            _ => Ok(self),
        }
    }
    fn replace(&mut self, arg: &Rc<RefCell<(Tree, bool)>>, par: &String) {
        match self {
            Tree::LEF(sym, box tmp) if sym != par => {
                tmp.replace(arg, par);
            }
            Tree::EEF(sym, box tmp) if sym != par => {
                tmp.replace(arg, par);
            }
            Tree::App(box fst, box snd) => {
                fst.replace(arg, par);
                snd.replace(arg, par);
            }
            Tree::Par(sym) if sym == par => {
                *self = Tree::Arg(arg.clone());
            }
            _ => {}
        }
    }
    fn translate(&self, lb: bool, rb: bool) -> String {
        match self {
            Tree::LEF(sym, box tmp) => {
                let xxx = format!("\\{} {}", sym, tmp.translate(false, rb && !rb));
                if rb { format!("({})", xxx) } else { xxx }
            }
            Tree::EEF(sym, box tmp) => {
                let xxx = format!("^{} {}", sym, tmp.translate(false, rb && !rb));
                if rb { format!("({})", xxx) } else { xxx }
            }
            Tree::Ellipsis => "...".to_string(),
            Tree::Int(i) => i.to_string(),
            Tree::Opr(_, name) => name.clone(),
            Tree::Cmp(_, name) => name.clone(),
            Tree::AOI(_, name, int) => {
                let xxx = format!("{} {}", name, int);
                if lb { format!("({})", xxx) } else { xxx }
            }
            Tree::ACI(_, name, int) => {
                let xxx = format!("{} {}", name, int);
                if lb { format!("({})", xxx) } else { xxx }
            }
            Tree::App(box fst, box snd) => {
                let xxx = format!("{} {}", fst.translate(lb && !lb, true), snd.translate(true, rb && !lb));
                if lb { format!("({})", xxx) } else { xxx }
            }
            Tree::Arg(arg) => {
                let (shr, _) = &*arg.borrow();
                shr.translate(lb, rb)
            }
            Tree::Par(par) => format!("${}", par),
            _ => panic!("invalid tree"),
        }
    }
}
fn process(input: String, map: &mut HashMap<String, Rc<RefCell<(Tree, bool)>>>) -> Result<(), String> {
    let (cmd, exp) = read(&input)?;
    if cmd.is_empty() || cmd == "#" {
        Ok(())
    } else if cmd.starts_with(':') {
        let mut res = Tree::parse(exp, Tree::Undefined, Tree::Undefined)?;
        for (par, arg) in map.iter() {
            res.replace(arg, par);
        }
        map.insert(cmd[1..].to_string(), Rc::new(RefCell::new((res, false))));
        Ok(())
    } else if cmd == "cal" {
        let mut res = Tree::parse(exp, Tree::Undefined, Tree::Undefined)?;
        for (par, arg) in map.iter() {
            res.replace(arg, par);
        }
        let res = res.calculate()?;
        eprint!("{}", *PS_RES);
        println!("{}", res.translate(false, false));
        map.insert("".to_string(), Rc::new(RefCell::new((res, true))));
        Ok(())
    } else if cmd == "dir" {
        for (par, _) in map.iter() {
            eprint!("{}", *PS_OUT);
            println!(":{}", par);
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

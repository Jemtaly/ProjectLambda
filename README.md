# ProjectLambda

ProjectLambda is a calculator / programming language based on lambda calculus.

## Compiling

```
git clone https://github.com/Jemtaly/ProjectLambda
cd ProjectLambda
make SOURCE=lambda_cbv.cpp           # Global variable Call-by-value (hereinafter referred to as CBV) version
make SOURCE=lambda_cbv.cpp USE_GMP=1 # CBV version using GNU MP Bignum Library
make SOURCE=lambda_cbn.cpp           # Global variable Call-by-need (hereinafter referred to as CBN) version (Experimental, see note below)
make SOURCE=lambda_cbn.cpp USE_GMP=1 # CBN version using GNU MP Bignum Library
```

## Commands

| instruction | usage |
| --- | --- |
| `cal FORMULA` | Calculate the formula. |
| `:VAR FORMULA` | Define a global variable/function. |
| `dir` | List all the defined functions/variables. |
| `clr` | Clear the defined functions/variables. |
| `end` or <kbd>eof</kbd> | Exit the program. |

## Syntax

The basic syntax of ProjectLambda is the same as the standard form of the lambda algorithm. Applications are assumed to be left associative: `M N P` and `((M N) P)` are equivalent. Other syntax is as follows:

| symbol | meaning |
| --- | --- |
| `\PAR EXPR` | Lambda expression, `PAR` is the formal parameter, `EXPR` is the body, the body of an abstraction extends as far right as possible. Example: `\x \y $y $x` => $\lambda x.\lambda y.y\ x$ |
| `EXPR \|PAR1 ARG1 \|PAR2 ARG2 ...` | This is a syntactic sugar for lambda expressions whose arguments are already determined. If you know haskell, think of it as something like `where` in that. <br/>`\|PAR` has lower priority than apply, but higher than `\PAR`. Example: `\y $x $y $z \|z + 1 2 \|x \a \b $t $a $b \|t +` => `\y ($x $y $z \|z (+ 1 2) \|x \a \b ($t $a $b \|t +))`. |
| `^PAR EXPR` | This is similar to `\PAR EXPR`, the difference is, the argument to `\PAR EXPR` is lazy-evaluated, while the argument to `^PAR EXPR` is eager-ealuated, which means its argument will be computed before they are brought into the function. |
|  `EXPR @PAR1 ARG1 @PAR2 ARG2 ...` | Syntactic sugar for eager-evaluated lambda expression. |
| `$ARG` | Formal parameter, used in lambda expressions. |
| `&VAR` | Call the global function/variable defined by `:VAR` instruction. (Only for CBN version, use `$VAR` instead in CBV version.) |
| `+` `-` `*` `/` `%` | Binary operators, in the form of operands-swapped prefix expressions, e.g., $(5-2)/3$ should be represented as `/ 3 (- 2 5)`. |
| `>` `<` `=` | Comparison operators, which takes four arguments, compares the first two arguments (as same as binary operaters, the operands should be swapped) and returns the third if the result is true and the fourth if the result is false. For example, `> 1 2 3 4` equals to `2 > 1 ? 3 : 4` in C. |
| `...` | Nil. As a function, it gets itself when applied to any argument. |

**Note:** *Spaces in expressions cannot be omitted.*

## Syntax details

*Unless otherwise specified, the examples below are for the CBV version.*

### First example

```
cal (\a \o \b $o $b $a) - 3 10
# output: 7
```

**Computational procedure:** `(\a \o \b $o $b $a) 10 - 3` => `(\o \b $o $b 10)` => `(\b - $b 10) 3` => `- 3 10` => `7`

### Lazy evaluation

```
cal (\x \y $y) ((\f $f $f) \f $f $f) 0
# output: 0
```

**Computational procedure:** `(\x \y $y) ((\f $f $f) \f $f $f) ((\a \o \b $o $b $a) 12 / 4)` => `(\y $y) 0` => `0`

**Explain:** The calculation of `(\f $f $f) \f $f $f` will cause the program to run indefinitely. However, since `\PAR EXPR` is lazy-evaluated, in `\x \y $y`, the formal parameter `$x` corresponding to `(\f $f $f) \f $f $f` is not used, so it is not actually calculated, thus we successfully get the expected result.

### Scope of formal parameters

```
cal (\x \x $x) 1 2
# output: 2
```

**Computational procedure:** `(\x \x $x) 1 2` => `(\x $x) 2` => `2`

**Explain:** When there are multiple parameters with the same name, the internal formal parameter will match the nearest actual parameter.

### Use of `|PAR`

This is a syntactic sugar for lambda expressions whose arguments are already determined. If you know haskell, think of it as something like `where` in that.

Examples:

- `(* (f x y z) (f x y z))` => `* $t $t |t f x y z`
- `(* (f x y z) (g x y z))` => `* $s $t |s f x y z |t g x y z`
- `(* (f (x y)) (g (x y)))` => `* $s $t |s f $u |t g $u |u x y`

### Difference between `\PAR EXPR` and `^PAR EXPR`

As mentioned earlier, the parameter of `\PAR EXPR` is lazy-evaluated, while the parameter of `^PAR EXPR` is eager-evaluated. Usually, we only need to use the former (because it avoids calculating parameters that do not need to be used). However, since in ProjectLambda, if the function is not applied, its internal content will not be calculated, so if you want to construct a tuple (`\o ... ... ...`) and output, then, with `\PAR EXPR` syntax, you will get a long list of raw results:

```
:LazyTriple \a \b \c \o $o $a $b $c
cal &LazyTriple (+ 0 0) (+ 1 1) (+ 2 2)
# output: \o $o (+ 0 0) (+ 1 1) (+ 2 2)
```

Obviously, this is not what we want, and if you use the syntax of `^PAR EXPR`, the parameters will be calculated first and then substituted into the function:

```
:EagerTriple ^a ^b ^c \o $o $a $b $c
cal &EagerTriple (+ 0 0) (+ 1 1) (+ 2 2)
# output: \o $o 0 2 4
```

In fact, using this syntax, if you have a lazy-evaluated infinite array (of the form `\o $o ... \o $o ... \o $o \o $o ... ...`), you can easily calculate and output the value of its first n elements through the following function:

```
:LazyCons \a \b \o $o $a $b
:car \list $list \a \b $a
:cdr \list $list \a \b $b
:EagerCons ^a ^b \o $o $a $b
:eval \n \lazylist = 0 $n ... (&EagerCons (&car $lazylist) (&eval (- 1 $n) (&cdr $lazylist)))
```

*Note: `...` is used here to denote an empty list.*

Example:

```
:fib \m \n &LazyCons $m (&fib $n (+ $m $n))
cal &fib 0 1
# output: \o $o 0 (&fib 1 (+ 0 1))
cal &eval 10 (&fib 0 1)
# output: \o $o 0 \o $o 1 \o $o 1 \o $o 2 \o $o 3 \o $o 5 \o $o 8 \o $o 13 \o $o 21 \o $o 34 ...
```

### Difference between CBN and CBV Version

The main difference between the CBN and CBV versions lies in the storage and calling of global variables: in the CBV version, global variables are stored in shared pointers, just like arguments in lambda calculus, so they are only evaluated once if needed. However, to avoid circular references, named functions in this version cannot directly recursively call themselves or undefined functions, they must use Y combinators to implement recursion, just like those anonymous functions. On the other hand, in the CBN version, global functions and variables are stored directly as unevaluated syntax trees, and a new copy is created and recalculated each time they are used. Therefore, named functions in this version can directly reference themselves or variables/functions that are not yet defined.

### Calculating the factorial of 99 using recursion

- Method A (Only available for CBV version)

```
:fact \n > 0 $n (* $n (&fact (- 1 $n))) 1
cal &fact 99
# output: 933262154439441526816992388562667004907159682643816214685929638952175999932299156089414639761565182862536979208272237582511852109168640000000000000000000000
```

- Method B (Fixed-point combinator)

```
:Y \g (\x $g ($x $x)) \x $g ($x $x)
:G \f \n > 0 $n (* $n ($f (- 1 $n))) 1
:fact &Y &G
cal &fact 99
# output: 933262154439441526816992388562667004907159682643816214685929638952175999932299156089414639761565182862536979208272237582511852109168640000000000000000000000
```

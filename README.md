# ProjectLambda

ProjectLambda is a calculator / programming language based on lambda calculus.

## Compiling

```
git clone https://github.com/Jemtaly/ProjectLambda
cd ProjectLambda
make SOURCE=lambda_full.cpp           # full version
make SOURCE=lambda_full.cpp USE_GMP=1 # full version using GNU MP Bignum Library
make SOURCE=lambda_lite.cpp           # lite version (removes support for the `!VAR` and `fmt` commands)
make SOURCE=lambda_lite.cpp USE_GMP=1 # lite version using GNU MP Bignum Library
```

## Commands

| instruction | usage |
| --- | --- |
| `cal FORMULA` | Calculate the formula. |
| `fmt FORMULA` | Format the formula. |
| `!VAR FORMULA` | Caculate the formula and save it in `!VAR`. |
| `&VAR FORMULA` | Format the formula and save it in `&VAR`. |
| `dir` | List all the defined functions/variables. |
| `clr` | Clear the defined functions/variables. |
| `end` or <kbd>eof</kbd> | Exit the program. |

## Syntax

Applications are assumed to be left associative: `M N P` and `((M N) P)` are equivalent.

| symbol | meaning |
| --- | --- |
| `\PAR EXPR` | Lambda expression, `PAR` is the formal parameter, `EXPR` is the body, the body of an abstraction extends as far right as possible. Example: `\x \y $y $x` => $\lambda x.\lambda y.y\ x$ |
| `EXPR \|PAR1 ARG1 \|PAR2 ARG2 ... ` | This is a syntactic sugar for lambda expressions whose arguments are already determined. If you know haskell, think of it as something like `where` in that. <br/>`\|PAR` has lower priority than apply, but higher than `\PAR`. Example: `\y $x $y $z \|z + 1 2 \|x \a \b $t $a $b \|t +` => `\y ($x $y $z \|z (+ 1 2) \|x \a \b ($t $a $b \|t +))`. |
| `^PAR EXPR` or `EXPR @PAR ARG` | These two are similar to `\PAR EXPR` and `EXPR \|PAR ARG`, but their arguments will not be lazy, which means, the arguments will be computed before they are brought into the function. |
| `$ARG` | Formal parameter, used in lambda expressions. |
| `&VAR` | Call the function/variable defined by `def` instruction. |
| `!VAR` | Call the function/variable defined by `set` instruction. |
| `+` `-` `*` `/` `%` | Binary operators, in the form of operands-swapped prefix expressions, e.g., $(5-2)/3$ should be represented as `/ 3 (- 2 5)`. |
| `>` `<` `=` | Comparison operators, which takes four arguments, compares the first two arguments (as same as binary operaters, the operands should be swapped) and returns the third if the result is true and the fourth if the result is false. For example, `> 1 2 3 4` equals to `2 > 1 ? 3 : 4` in C. |
| `...` | Nil. As a function, it gets itself when applied to any argument. |

**Note:** The spaces between the above symbols should not be omitted.

## Examples

### First example

```
cal (\a \o \b $o $b $a) - 3 10
# output: 7
```

**Computational procedure:** `(\a \o \b $o $b $a) 10 - 3` => `(\o \b $o $b 10)` => `(\b - $b 10) 3` => `- 3 10` => `7`

### Lazy evaluation

```
cal (\x \y $y) ((\f $f $f) \f $f $f) 0
# output: 3
```

**Computational procedure:** `(\x \y $y) ((\f $f $f) \f $f $f) ((\a \o \b $o $b $a) 12 / 4)` => `(\y $y) 0` => `0`

**Explain:** `(\f $f $f) \f $f $f` is the smallest term that has no normal form, the term reduces to itself in a single β-reduction, and therefore the reduction process will never terminate (until the stack overflows). However, since we use lazy evaluation, it will not be evaluated until it is called, so the program will not be stuck.

### Scope of formal parameters

```
cal (\x \x $x) 1 2
# output: 2
```

**Computational procedure:** `(\x \x $x) 1 2` => `(\x $x) 2` => `2`

**Explain:** When there are multiple parameters with the same name, the internal formal parameter will match the nearest actual parameter.

### Difference between `&VAR` and `!VAR`

```
!x 100
!y * !x
&y * !x
cal !y 2
# output: 200
cal &y 2
# output: 200
!x + 50 !x
cal !y 2
# output: 200
cal &y 2
# output: 300
```

**Explain:** `!y * !x` calculate the formula and set `!y` to `* 100`, while `&y * !x` set `&y` to literally `* !x`, so whenever you call `&y`, it will be recalculated, therefore, after statement `!x + 50 !x` change the value of `!x` to 150, the result of `cal &y 2` will be changed to `300`.

### Calculating the factorial of 99 using recursion

- Method A

```
&fact \n > 0 $n (* $n (&fact (- 1 $n))) 1
cal &fact 99
# output: 933262154439441526816992388562667004907159682643816214685929638952175999932299156089414639761565182862536979208272237582511852109168640000000000000000000000
```

- Method B (Fixed-point combinator)

```
!Y \g (\x $g ($x $x)) \x $g ($x $x)
!G \f \n > 0 $n (* $n ($f (- 1 $n))) 1
!fact !Y !G
cal !fact 99
# output: 933262154439441526816992388562667004907159682643816214685929638952175999932299156089414639761565182862536979208272237582511852109168640000000000000000000000
```

### Use of `|PAR`

This is a syntactic sugar for lambda expressions whose arguments are already determined. If you know haskell, think of it as something like `where` in that.

Examples:

- `(* (f x y z) (f x y z))` => `* $t $t |t f x y z`
- `(* (f x y z) (g x y z))` => `* $s $t |s f x y z |t g x y z`
- `(* (f (x y)) (g (x y)))` => `* $s $t |s f $u |t g $u |u x y`

### Difference between `\PAR EXPR` and `^PAR EXPR`

As mentioned earlier, the parameters of `\PAR EXPR` are lazily evaluated, while the parameters of `^PAR EXPR` are not lazily evaluated. Usually, we only need to use the former (because it avoids calculating parameters that do not need to be used). However, since in ProjectLambda, if the function is not applied, its internal content will not be calculated, so if you want to construct a tuple (`\o ... ... ...`) and output, then, if constructed using `\PAR EXPR`, you will get a long list of raw results:

```
&LazyTriple \a \b \c \o $o $a $b $c
cal &LazyTriple (+ 0 0) (+ 1 1) (+ 2 2)
# output: \o $o (+ 0 0) (+ 1 1) (+ 2 2)
```

Obviously, this is not what we want, and if you use the syntax of `^PAR EXPR`, the parameters will be calculated first and then substituted into the function:

```
&NonLazyTriple \a \b \c \o $o $a $b $c
cal &NonLazyTriple (+ 0 0) (+ 1 1) (+ 2 2)
# output: \o $o 0 2 4
```

In fact, using this syntax, if you have a lazy-evaluated infinite array (of the form `\o $o ... \o $o ... \o $o \o $o ... ... ), you can quickly calculate the value of its first n elements through the following function:

```
&LazyCons \a \b \o $o $a $b
&car \list $list \a \b $a
&cdr \list $list \a \b $b
&NonLazyCons ^a ^b \o $o $a $b
&eval \n \lazylist = 0 $n ... (&NonLazyCons (&car $lazylist) (&eval (- 1 $n) (&cdr $lazylist)))
```

*Note: `...` is used here to denote an empty list.*

Example:

```
&fib \m \n &LazyCons $m (&fib $n (+ $m $n))
cal &fib 0 1
# output: \o $o 0 (&fib 1 (+ 0 1))
cal &eval 10 (&fib 0 1)
# output: \o $o 0 \o $o 1 \o $o 1 \o $o 2 \o $o 3 \o $o 5 \o $o 8 \o $o 13 \o $o 21 \o $o 34 ...
```

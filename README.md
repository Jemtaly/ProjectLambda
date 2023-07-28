# ProjectLambda

ProjectLambda is a calculator / programming language based on lambda calculus.

## Compiling and Running

```
git clone https://github.com/Jemtaly/ProjectLambda
cd ProjectLambda
clang++ lambda.cpp -std=c++17 -Os -o lambda.exe
./lambda.exe
```

*Add `-DUSE_GMP` option at compile time if you want to use the GNU MP bignum library for large integer operations.*

## Commands

| instruction | usage |
| --- | --- |
| `cal FORMULA` | Calculate the formula. |
| `fmt FORMULA` | Format the formula. |
| `set VAR FORMULA` | Caculate the formula and save it in `!VAR`. |
| `def VAR FORMULA` | Format the formula and save it in `&VAR`. |
| `dir` | List all the defined functions/variables. |
| `clr` | Clear the defined functions/variables. |
| `end` or <kbd>eof</kbd> | Exit the program. |

## Syntax

| symbol | meaning |
| --- | --- |
| `(...)` | Applications are assumed to be left associative: `M N P` and `((M N) P)` are equivalent. |
| `ARG: EXPR` | Lambda expression, `ARG` is the formal parameter, `EXPR` is the body, the body of an abstraction extends as far right as possible. Example: `x: y: $y $x` => $\lambda x.\lambda y.y\ x$. |
| `$ARG` | Formal parameter, used in lambda expressions. |
| `&VAR` | Call the function/variable defined by `def` instruction. |
| `!VAR` | Call the function/variable defined by `set` instruction. |
| `+` `-` `*` `\` `%` | Binary operators, in the form of operands-swapped prefix expressions, e.g., $(5-2)/3$ should be represented as `/ 3 (- 2 5)`. |
| `>` `<` `=` | Comparison operators, which takes four arguments, compares the first two arguments (as same as binary operaters, the operands should be swapped) and returns the third if the result is true and the fourth if the result is false. For example, `> 1 2 3 4` equals to `2 > 1 ? 3 : 4` in C. |

**Note:** The spaces between the above symbols should not be omitted.

## Examples

### First example

```
cal (x: y: $y $x) world! Hello,
# output: Hello, world!
```

**Computational procedure:** `(x: y: $y $x) world! Hello,` => `(y: $y world!) Hello,` => `Hello, world!`

### Lazy evaluation

```
cal (x: y: $y) ((f: $f $f) f: $f $f) ((a: o: b: $o $b $a) 12 / 4)
# output: 3
```

**Computational procedure:** `(x: y: $y) ((f: $f $f) f: $f $f) ((a: o: b: $o $b $a) 12 / 4)` => `(y: $y) ((a: o: b: $o $b $a) 12 / 4)` => `(a: o: b: $o $b $a) 12 / 4` => `/ 4 12` => `3`

**Explain:** `(f: $f $f) f: $f $f` is the smallest term that has no normal form, the term reduces to itself in a single β-reduction, and therefore the reduction process will never terminate (until the stack overflows). However, since we use lazy evaluation, it will not be evaluated until it is called, so the program will not be stuck.

### Scope of formal parameters

```
cal (x: y: x: $x $y) a b
# output: b
```

**Computational procedure:** `(x: y: x: $x $y) a b` => `(y: x: $x $y) b` => `(x: $x) b` => `b`

**Explain:** When there are multiple parameters with the same name, the internal formal parameter will match the nearest actual parameter.

### Difference between `def` and `set`

```
set x 100
set y * !x
def y * !x
cal !y 2
# output: 200
cal &y 2
# output: 200
set x + 50 !x
cal !y 2
# output: 200
cal &y 2
# output: 300
```

**Explain:** `set y * !x` calculate the formula and set `!y` to `* 100`, while `def y * !x` set `&y` to literally `* !x`, so whenever you call `&z`, it will be recalculated, therefore, after `set x + 50 !x` change the value of `!x` to 150, the result of `&y 2` will be changed to `300`.

### Calculating the factorial of 99 using recursion

- Method A

```
def fact n: > 0 $n (* $n (&fact (- 1 $n))) 1
cal &fact 99
# output: 933262154439441526816992388562667004907159682643816214685929638952175999932299156089414639761565182862536979208272237582511852109168640000000000000000000000
```

- Method B (Fixed-point combinator)

```
set Y g: (x: $g ($x $x)) x: $g ($x $x)
set G f: n: = 0 $n 1 (* $n ($f (- 1 $n)))
set fact !Y !G
cal !fact 99
# output: 933262154439441526816992388562667004907159682643816214685929638952175999932299156089414639761565182862536979208272237582511852109168640000000000000000000000
```

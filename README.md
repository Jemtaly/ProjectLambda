# ProjectLambda

ProjectLambda is a calculator / programming language based on lambda calculus.

## Compiling

```
git clone https://github.com/Jemtaly/ProjectLambda
cd ProjectLambda
./build.sh lambda_full.cpp # full version
./build.sh lambda_lite.cpp # version that removes support for the `!VAR` and `fmt` commands
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

| symbol | meaning |
| --- | --- |
| `(...)` | Applications are assumed to be left associative: `M N P` and `((M N) P)` are equivalent. |
| `\PAR EXPR` | Lambda expression, `PAR` is the formal parameter, `EXPR` is the body, the body of an abstraction extends as far right as possible. Example: `\x \y $y $x` => $\lambda x.\lambda y.y\ x$ |
| `EXPR \|PAR1 ARG1 \|PAR2 ARG2 ... ` | This is a syntactic sugar for lambda expressions whose arguments are already determined. If you know haskell, think of it as something like `where` in that. <br/>`\|PAR` has lower priority than apply, but higher than `\PAR`. Example: `\y $x $y $z \|z + 1 2 \|x \a \b $t $a $b \|t +` => `\y ($x $y $z \|z (+ 1 2) \|x (\a \b ($t $a $b \|t +)))`. |
| `$ARG` | Formal parameter, used in lambda expressions. |
| `&VAR` | Call the function/variable defined by `def` instruction. |
| `!VAR` | Call the function/variable defined by `set` instruction. |
| `+` `-` `*` `/` `%` | Binary operators, in the form of operands-swapped prefix expressions, e.g., $(5-2)/3$ should be represented as `/ 3 (- 2 5)`. |
| `>` `<` `=` | Comparison operators, which takes four arguments, compares the first two arguments (as same as binary operaters, the operands should be swapped) and returns the third if the result is true and the fourth if the result is false. For example, `> 1 2 3 4` equals to `2 > 1 ? 3 : 4` in C. |

**Note:** The spaces between the above symbols should not be omitted.

## Examples

### First example

```
cal (\a \op \b $op $b $a) - 3 10
# output: 7
```

**Computational procedure:** `(\a \op \b $op $b $a) 10 - 3` => `(\op \b $op $b 10)` => `(\b - $b 10) 3` => `- 3 10` => `7`

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

### `|PAR`

This is a syntactic sugar for lambda expressions whose arguments are already determined. If you know haskell, think of it as something like `where` in that.

Examples:

- `(* (f x y z) (f x y z))` => `* $t $t |t f x y z`
- `(* (f x y z) (g x y z))` => `* $s $t |s f x y z |t g x y z`
- `(* (f (x y)) (g (x y)))` => `* $s $t |s f $u |t g $u |u x y`

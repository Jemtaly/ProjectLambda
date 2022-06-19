# ProjectLambda

ProjectLambda is a calculator / programming language based on lambda calculus.

## Compiling and Using

Compile the c++ source code, then run it directly from the command line.

## Commands

| command | usage |
| --- | --- |
| `cal FORMULA` | Calculate the formula. |
| `cal FORMULA` | Format the formula. |
| `set VAR FORMULA` | Caculate the formula and save it in VAR. |
| `def VAR FORMULA` | Format the formula and save it in VAR. |
| `dir` | List all the defined functions/variables. |
| `clr` | Clear the defined functions/variables. |

## Syntax

| syntax | meaning |
| --- | --- |
| `[...]` | An lambda expression with single argument. |
| `(...)` | Since applications are assumed to be left associative, you only have to use brackets in case like `f (g a)`, rather than in case like `((f g) b)`. |
| `$a` `$b` `...` | Formal parameters, use `$a` for the argument to the innermost lambda expression, use `$b` for the second innermost one, and so on ... Example: `[[$a $b] $a] x` => `[$a x] x` => `x x`. |
| `&FUNCTION` | Call the function/variable defined by `def` instruction. |
| `!FUNCTION` | Call the function/variable defined by `set` instruction. |
| `+` `-` `*` `\` `%` | Binary operators, in the form of prefix expressions, e.g., `(5 - 2) / 3` should be represented as `/ 3 (- 2 5)`. |
| `>` `<` `=` | Comparison operators, which takes four arguments, compares the first two arguments and returns the third if the result is true and the fourth if the result is false. For example, `> 1 2 3 4` equals to `2 > 1 ? 3 : 4` in C. |
| `+:n` `-:n` `*:n` `\:n` `%:n` `>:n` `<:n` `=:n` | Equivalent to `+ n` `- n` ... Notice that `n` must be a number, not an expression. |

## Examples

### First example

```
cal [[$a $b]] 100 (- 50)
# output: 50
# [[$a $b]] 100 (- 50) => [$a 100] (- 50) => - 50 100 => -:50 100 => 50
```

### Lazy evaluation

```
cal [[$a]] ([$a $a] [$a $a]) ([$a $a] (Hello World))
# output: Hello World (Hello World)
# Because of the undecidability of combinatorial calculus, "[$a $a] [$a $a]" will cause a infinite loop, but thanks to the mechanism of lazy evaluation, it is not really calculated, so the formula will be calculated successfully.
```

### Difference between `def` and `set`

```
set X 100
set Y * !X
def Z * !X
cal !Y 2
# output: 200
cal &Z 2
# output: 200
set X + 50 !X
cal !Y 2
# output: 200
cal &Z 2
# output: 300
# "set Y * !X" calculate the formula and set !Y to "*:100", while "def Z * !X" set &Z to literally "* !X", so whenever you call &Z, it will be recalculated, therefore, when "set X + 50 !X" changed the value of !X to 150, the result of "&Z 2" changed to 300.
```

### Calculating the factorial of 99 using recursion

```
def FACTORIAL [>:0 $a (* $a (&FACTORIAL (-:1 $a))) 1]
cal &FACTORIAL 99
# output: 933262154439441526816992388562667004907159682643816214685929638952175999932299156089414639761565182862536979208272237582511852109168640000000000000000000000
```

### Convert 114514 to ternary

```
def BASE [[$a $a] [[< $c $a $a ($b $b (/ $c $a) (% $c $a))]]]
cal &BASE 3 114514
# output: 1 2 2 1 1 0 0 2 0 2 1
```

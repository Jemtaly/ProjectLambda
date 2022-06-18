# ProjectLambda

ProjectLambda is a calculator / programming language based on lambda calculus.

## Compiling and Using

Compile the c++ source code, then run it directly from the command line.

## Commands

| command | usage |
| --- | --- |
| `calc ...` | Calculate the statement. |
| `set FUNCTION/VAR FORMULA` | Set a variable/function. |
| `def FUNCTION/VAR FORMULA` | Similar to `set`, but the formula will not be calculated until it's used. |
| `list` | List all the defined functions. |
| `del FUNCTION1 FUNCTION2 ...` | Delete the defined functions. |

## Syntax

| syntax | meaning |
| --- | --- |
| `[...]` | An lambda expression with single argument. |
| `(...)` | Since applications are assumed to be left associative, you only have to use brackets in case like `f (g a)`, rather than in case like `((f g) b)`. |
| `$a` `$b` `...` | Formal parameters, use `$a` for the argument to the innermost lambda expression, use `$b` for the second innermost one, and so on ... Example: `[[$a $b] $a] x` => `[$a x] x` => `x x`. |
| `&FUNCTION` | Call the function. |
| `+` `-` `*` `\` `%` | Binary operators, in the form of prefix expressions, e.g., `(5 - 2) / 3` should be represented as `/ 3 (- 2 5)`. |
| `>` `<` `=` | Comparison operators, which takes four arguments, compares the first two arguments and returns the third if the result is true and the fourth if the result is false. For example, `> 1 2 3 4` equals to `2 > 1 ? 3 : 4` in C. |
| `+:n` `-:n` `*:n` `\:n` `%:n` `>:n` `<:n` `=:n` | Equivalent to `+ n` `- n` ... Notice that `n` must be a number, not an expression. |

## Examples

### Reverse function application

```
calc [[$a $b]] 100 (- 50)
# output: 50
# explain: [[$a $b]] 100 (- 50) => [$a 100] (- 50) => - 50 100 => -:50 100 => 50
```

### Calculating the factorial of 99

```
def FACTORIAL [>:1 $a (* $a (&FACTORIAL (-:1 $a))) 1]
calc &FACTORIAL 99
# output: 933262154439441526816992388562667004907159682643816214685929638952175999932299156089414639761565182862536979208272237582511852109168640000000000000000000000
```

### Convert 114514 to ternary

```
def BASE [[$a $a] [[< $c $a $a ($b $b (/ $c $a) (% $c $a))]]]
calc &BASE 3 114514
# output: 1 2 2 1 1 0 0 2 0 2 1
```

### Lazy evaluation

```
set X 100
def Y *:2 &X
calc &Y
# output: 200
def X +:25 25
calc &Y
# output: 100
```

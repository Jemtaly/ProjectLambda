# ProjectLambda

ProjectLambda is a calculator / programming language based on lambda calculus.

## Compiling and Using

Compile the c++ source code, then run it directly from the command line.

## Commands

| command | usage |
| --- | --- |
| `def FUNCTION ...` | to define a function |
| `calc ...` | to calculate the statement |
| `list` | to list all the defined functions |
| `del FUNCTION1 FUNCTION2 ...` | to delete the defined functions |
| `exit` | to exit the calculator|

## Syntax

| syntax | meaning |
| --- | --- |
| `[...]` | an lambda expression with single argument |
| `(...)` | Since applications are assumed to be left associative, you only have to use brackets in case like `f (g a)`, rather than in case like `((f g) b)`. |
| `$a` `$b` `...` | Formal parameters, use `$a` for the argument to the innermost lambda expression, use `$b` for the second innermost one, and so on ... Example: `[[$a $b] $a] x` => `[$a x] x` => `x x` |
| `&FUNCTION` | to call the function |
| `+` `-` `*` `\` `%` | Binary operators, in the form of prefix expressions, e.g., `(5 - 2) / 3` should be represented as `/ 3 (- 2 5)`. |
| `+:n` `-:n` `*:n` `\:n` `%:n` | Equivalent to `+ n` `- n` `* n` `\ n` `% n`, notice that `n` must be a number, not an expression. |
| `>` `<` `=` | Comparison operators, which takes four arguments, compares the first two arguments and returns the third if the result is true and the fourth if the result is false. For example, `> 1 2 3 4` equals to `2 > 1 ? 3 : 4` in C. |

## Examples

### Calculating the factorial of 99

```
def FACTORIAL [$a $a] [[>:1 $a (* $a ($b $b (-:1 $a))) 1]]
calc &FACTORIAL 99
```

### Convert 114514 to ternary

```
def BASE [[$a $a] [[< $c $a $a ($b $b (/ $c $a) (% $c $a))]]]
calc &BASE 3 114514
```

# Y combinator
:Y \g $g ($x $x) |x \x $g ($x $x)

# True and false
:T \T \F $T
:F \T \F $F

# Logical operators (Id, Not, And, Or, Xor)
:= \c $c $T $F
:~ \c $c $F $T
:& \c \d $c $d $F
:| \c \d $c $T $d
:^ \c \d $c $= $~ $d

# List operators (Cons, Car, Cdr, Drop, Map, Scan, Filter, ZipWith)
:con \a \b \o $o $a $b
:car \l $l \a \_ $a
:cdr \l $l \_ \d $d
:drp $Y \z \n \l = 0 $n $l ($l \_ \d $z (- 1 $n) $d)
:map $Y \z \f \l $l \a \d $con ($f $a) ($z $f $d)
:scn $Y \z \f \x \l $l \a \d $con $x ($z $f ($f $x $a) $d)
:flt $Y \z \c \l $l \a \d $c $a ($con $a ($z $c $d)) ($z $c $d)
:zip $Y \z \f \l \m $l \a \b $m \c \d $con ($f $a $c) ($z $f $b $d)

# Non-lazy list constructor
:CON ^a ^b \o $o $a $b

# Evaluate the first n elements of a list
:EVL $Y \z \n \l = 0 $n ... ($l \a \d $CON $a ($z (- 1 $n) $d))

# Three different implementations of the Fibonacci sequence
:fib $Y \z \a \b $con $a ($z $b (+ $a $b))
cal $EVL 1024 $ | $fib 0 1
:fib \a \b $Y \l $con $a ($scn + $b $l)
cal $EVL 1024 $ | $fib 0 1
:fib \a \b $Y \l $con $a ($con $b ($zip + $l ($cdr $l)))
cal $EVL 1024 $ | $fib 0 1

# 2D space vector operations (Non-lazy construct, Add, Multiply)
:VEC ^x ^y \o $o $x $y
:add \u \v $u \a \b $v \c \d $VEC (+ $a $c) (+ $b $d)
:mul \u \v $u \a \b $v \c \d $VEC (- (* $b $d) (* $a $c)) (+ (* $a $d) (* $b $c))

# Regular paper folding sequence
:lft $VEC 0 (- 1 0)
:rgt $VEC 0 (+ 1 0)
:ins $Y \z \r $con $lft ($r \a \p $con $a ($con $rgt ($p \b \q $con $b ($z $q))))
:rpf $Y \z $ins $z

# Generate first 1024 points on the dragon curve from the regular paper folding sequence
:dir $scn $mul ($VEC 1 0) $rpf
:pos $scn $add ($VEC 0 0) $dir
cal $EVL 1024 $pos

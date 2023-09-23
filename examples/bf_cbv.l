:T \x \y $x
:F \x \y $y
:Mem ^cur ^lft ^rgt \ch $ch $cur $lft $rgt
:State ^mem ^in ^out \ch $ch $mem $in $out
:Cons ^head ^tail \ch $ch $head $tail
:Instrs ^head ^tail \fb \ch $ch $head $tail
:InsNil \fb \ch $fb

:Y \g (\x $x $x) \x $g ($x $x)
:fold \instrs \state $instrs $state \xhead \xtail &fold $xtail ($xhead $state)
:succ \n = 255 $n 0 (+ 1 $n)
:pred \n = 0 $n 255 (- 1 $n)
:< \state $state \mem \in \out $mem \cur \lft \rgt $lft \lhead \ltail &State (&Mem $lhead $ltail (&Cons $cur $rgt)) $in $out
:> \state $state \mem \in \out $mem \cur \lft \rgt $rgt \rhead \rtail &State (&Mem $rhead (&Cons $cur $lft) $rtail) $in $out
:+ \state $state \mem \in \out $mem \cur \lft \rgt &State (&Mem (&succ $cur) $lft $rgt) $in $out
:- \state $state \mem \in \out $mem \cur \lft \rgt &State (&Mem (&pred $cur) $lft $rgt) $in $out
:. \state $state \mem \in \out $mem \cur \lft \rgt &State $mem $in (&Cons $cur $out)
:, \state $state \mem \in \out $mem \cur \lft \rgt $in \ihead \itail &State (&Mem $ihead $lft $rgt) $itail $out
:[] \instrs (&Y \fp \state $state \mem \in \out $mem \cur \lft \rgt = 0 $cur $state ($fp ($fun $state))) |fun &fold $instrs

:inf \ch $ch 0 &inf
:run \instrs \in (&fold $instrs (&State (&Mem 0 &inf &inf) $in ...)) \mem \in \out $out

# Example: Hello world!
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&[] $ |
#     &Instrs (&>) $ |
#     &Instrs (&+) $ |
#     &Instrs (&+) $ |
#     &Instrs (&+) $ |
#     &Instrs (&+) $ |
#     &Instrs (&[] $ |
#         &Instrs (&>) $ |
#         &Instrs (&+) $ |
#         &Instrs (&+) $ |
#         &Instrs (&>) $ |
#         &Instrs (&+) $ |
#         &Instrs (&+) $ |
#         &Instrs (&+) $ |
#         &Instrs (&>) $ |
#         &Instrs (&+) $ |
#         &Instrs (&+) $ |
#         &Instrs (&+) $ |
#         &Instrs (&>) $ |
#         &Instrs (&+) $ |
#         &Instrs (&<) $ |
#         &Instrs (&<) $ |
#         &Instrs (&<) $ |
#         &Instrs (&<) $ |
#         &Instrs (&-) $ |
#         &InsNil) $ |
#     &Instrs (&>) $ |
#     &Instrs (&+) $ |
#     &Instrs (&>) $ |
#     &Instrs (&+) $ |
#     &Instrs (&>) $ |
#     &Instrs (&-) $ |
#     &Instrs (&>) $ |
#     &Instrs (&>) $ |
#     &Instrs (&+) $ |
#     &Instrs (&[] $ |
#         &Instrs (&<) $ |
#         &InsNil) $ |
#     &Instrs (&<) $ |
#     &Instrs (&-) $ |
#     &InsNil) $ |
# &Instrs (&>) $ |
# &Instrs (&>) $ |
# &Instrs (&.) $ |
# &Instrs (&>) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&.) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&.) $ |
# &Instrs (&.) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&.) $ |
# &Instrs (&>) $ |
# &Instrs (&>) $ |
# &Instrs (&.) $ |
# &Instrs (&<) $ |
# &Instrs (&-) $ |
# &Instrs (&.) $ |
# &Instrs (&<) $ |
# &Instrs (&.) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&.) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&.) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&-) $ |
# &Instrs (&.) $ |
# &Instrs (&>) $ |
# &Instrs (&>) $ |
# &Instrs (&+) $ |
# &Instrs (&.) $ |
# &Instrs (&>) $ |
# &Instrs (&+) $ |
# &Instrs (&+) $ |
# &Instrs (&.) $ |
# &InsNil

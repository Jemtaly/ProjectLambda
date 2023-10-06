:Mem ^cur ^lft ^rgt \op $op $cur $lft $rgt
:State ^mem ^in ^out \op $op $mem $in $out
:Cons ^head ^tail \op $op $head $tail
:Instrs ^head ^tail \df \op $op $head $tail
:Instop \df \op $df

:Y \g (\x $x $x) \x $g ($x $x)
:fold $Y \fold \instrs \state $instrs $state \xhead \xtail $fold $xtail ($xhead $state)
:succ \n = 255 $n 0 (+ 1 $n)
:pred \n = 0 $n 255 (- 1 $n)
:< \state $state \mem \in \out $mem \cur \lft \rgt $lft \lhead \ltail $State ($Mem $lhead $ltail ($Cons $cur $rgt)) $in $out
:> \state $state \mem \in \out $mem \cur \lft \rgt $rgt \rhead \rtail $State ($Mem $rhead ($Cons $cur $lft) $rtail) $in $out
:+ \state $state \mem \in \out $mem \cur \lft \rgt $State ($Mem ($succ $cur) $lft $rgt) $in $out
:- \state $state \mem \in \out $mem \cur \lft \rgt $State ($Mem ($pred $cur) $lft $rgt) $in $out
:. \state $state \mem \in \out $mem \cur \lft \rgt $State $mem $in ($Cons $cur $out)
:, \state $state \mem \in \out $mem \cur \lft \rgt $in \ihead \itail $State ($Mem $ihead $lft $rgt) $itail $out
:[] \instrs ($Y \fp \state $state \mem \in \out $mem \cur \lft \rgt = 0 $cur $state ($fp ($fun $state))) |fun $fold $instrs

:inf $Y \inf \op $op 0 $inf
:run \instrs \in ($fold $instrs ($State ($Mem 0 $inf $inf) $in ...)) \mem \in \out $out

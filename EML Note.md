
# The EML (Exp - Minus - Log) by Andrzej Odrzywołek

We have a function operator $ eml(x, y) := e^x - \ln{y} $ . The space $ M $ initially contains $ \lim_{a \to 1^+}{a} $ . If $ M $ contains $ p $ and $ q $ ,then $ eml(p,q) $ is added to $ M $ .

## Operators

$$
\begin{align*}
eml(x, y) &:= e^x - \ln{y} \\
exp(x) &= e^x = eml(x, 1) \\
log(x) &= \ln{x} = eml(1, exp(eml(1, x))) \\
sub(x, y) &= x - y = eml(log(x), exp(y)) \\
neg(x) &= -x = sub(0, x) \\
add(x, y) &= x + y = sub(x, neg(y)) \\
rec(x) &= \frac{1}{x} = exp(neg(log(x))) \\
mul(x, y) &= x \cdot y = exp(add(log(x), log(y))) \\
div(x, y) &= x / y = mul(x, rec(y)) \\
pow(x, y) &= x^y = exp(mul(log(x), y)) \\
rot(x, y) &= \sqrt[y]{x} = pow(x, rec(y)) \\
cos(x) &= \cos{x} = div(add(exp(mul(i, x)), exp(mul(-i, x))), 2) \\
sin(x) &= \sin{x} = cos(sub(x, π/2)) \\
tan(x) &= \tan{x} = div(sin(x), cos(x)) \\
atan(x) &= \arctan{x} = div(log(div(add(1, mul(i, x)), add(1, mul(-i, x)))), 2i)
\end{align*}
$$

## Constants

$$
\begin{align*}
1 &:= \lim_{a \to 1^+}{a} \\
e &= exp(1) \\
0 &= log(1) \\
-1 &= neg(1) \\
2 &= add(1, 1) \\
i &= rot(-1, 2) \\
-i &= div(1, i) \\
\pi &= div(log(-1), i) \\
\frac{\pi}{2} &= div(π, 2)
\end{align*}
$$

$$
\begin{align}

\text{program} &\to [function]^* \\

\text{function} &\to func ([func\_params]) [statement] \\

\text{func\_params} &\to [[identifier],]^*[identifier]\\

\text{statement} &\to 
\begin{cases}
 \{ [statement]^* \} \\
 if ([expression]) [statement] \\
 while ([expression]) [statement] \\
 return \space [expression]; \\
 [d \_ type] \space [identifier] [=[expression]]^?; \\
 [expression]; \\
 ;
\end{cases} \\

\text{d\_type} &\to 
\begin{cases}
 int\_8 \\
 int\_16 \\
 int\_32 \\
 int\_64 \\
 char \\
 [identifier] \\
\end{cases} \\

\text{expression} &\to
\begin {cases}
    [int\_literal] \\
    [identifier] \\
    ([expression]) \\ % parenthesis
    [expression]([func\_call\_params]) \\ % function call
    [binary\_expression] \\
    [unary\_expression] \\
\end {cases}\\

\text{func\_call\_params} &\to
\begin {cases}
    \varepsilon \\
    [expression] \\
    [expression][,[expression]]^*
\end {cases} \\

\text{binary\_expression} &\to
\begin {cases}
    [expression] = [expression] \\
    [expression] + [expression] \\
    [expression] - [expression] \\
    [expression] * [expression] \\
    [expression] / [expression] \\
\end {cases} \\

\text{unary\_expression} &\to
\begin {cases}
    +[expression] \\
    -[expression]
\end{cases} \\

\text{identifier} &\to [a-zA-Z][\text{a-zA-Z0-9\_}]^* \\

\text{int\_literal} &\to
\begin {cases}
    \text{[0-9]}^+ \\
    \text{0x[0-9a-fA-F]}^+ \\
    \text{0b[01]}^+
\end {cases}

\end{align}
$$
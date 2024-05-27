$$
\begin{align}

\text{program} &\to [statement]^* \\

\text{statement} &\to
\begin {cases}
    \{ [statement]^* \} \\
    if ([expression]) [statement] [else [expression]]^? \\
    while ([expression]) [statement] \\
    exit([expression]) \\
    [d\_type]\space[identifier]; \\
    [d\_type]\space[identifier] = [expression]; \\
    [identifier] = [expression]; \\
    func\space[d\_type]([func\_params]) [statement] \\
    [expression]([func\_call\_params]) \\ % function call
\end {cases} \\

\text{expression} &\to
\begin {cases}
    [int\_literal] \\
    [identifier] \\
    ([expression]) \\ % parenthesis
    [expression]([func\_call\_params]) \\ % function call
    [binary\_expression] \\
\end {cases} \\

\text{func\_call\_params} &\to
\begin {cases}
    \varepsilon \\
    [expression] \\
    [expression],[func\_call\_params]^* \\
\end {cases} \\

\text{func\_params} &\to
\begin{cases}
    \varepsilon \\
    [d\_type]\space[identifier] \\
    [d\_type]\space[identifier],[func\_params] \\
\end{cases} \\


\text{binary\_expression} &\to
\begin {cases}
    [expression] + [expression] \\
    [expression] - [expression] \\
    [expression] * [expression] \\
    [expression] / [expression] \\
    [expression] \% [expression] \\
    [expression] > [expression] \\
    [expression] >= [expression] \\
    [expression] < [expression] \\
    [expression] <= [expression] \\
    [expression] == [expression] \\
\end {cases} \\

\text{int\_literal} &\to
\begin {cases}
    \text{[0-9]}^+ \\
    \text{0x[0-9a-fA-F]}^+ \\
    \text{0b[01]}^+
\end {cases} \\

\text{identifier} &\to [a-zA-Z\_][\text{a-zA-Z0-9\_}]^* \\

\text{d\_type} &\to
\begin{cases}
 void \\
 int\_16 \\
 int\_64 \\
\end{cases} \\

\end{align}
$$

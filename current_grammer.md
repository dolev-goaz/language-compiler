$$
\begin{align}

\text{program} &\to [statement]^* \\

\text{statement} &\to
\begin {cases}
    exit([expression]) \\
    [d\_type]\space[identifier]; \\
    [d\_type]\space[identifier] = [expression];
\end {cases} \\

\text{expression} &\to
\begin {cases}
    [int\_literal] \\
    [identifier] \\
\end {cases} \\

\text{int\_literal} &\to
\begin {cases}
    \text{[0-9]}^+ \\
    \text{0x[0-9a-fA-F]}^+ \\
    \text{0b[01]}^+
\end {cases} \\

\text{identifier} &\to [a-zA-Z][\text{a-zA-Z0-9\_}]^* \\

\text{d\_type} &\to
\begin{cases}
 int\_8 \\
 int\_16 \\
 int\_32 \\
 int\_64 \\
 char \\
\end{cases} \\

\end{align}
$$
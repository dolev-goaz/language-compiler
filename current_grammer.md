$$
\begin{align}

\text{program} &\to [statement]^* \\

\text{statement} &\to exit([expression]) \\

\text{expression} &\to
\begin {cases}
    [int\_literal] \\
    [identifier] \\
\end {cases}\\

\text{identifier} &\to [a-zA-Z][\text{a-zA-Z0-9\_}]^* \\

\text{int\_literal} &\to \text{[0-9]}^+ \\

\end{align}
$$
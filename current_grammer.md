$$
\begin{align}

\text{program} &\to [statement]^* \\

\text{statement} &\to exit([expression]) \\

\text{expression} &\to [int\_literal] \\

\text{int\_literal} &\to
\begin {cases}
    \text{[0-9]}^+ \\
    \text{0x[0-9a-fA-F]}^+ \\
    \text{0b[01]}^+
\end {cases}

\end{align}
$$
## MarkTeX

MarkTeX is a small transpiler to process a custom syntax that would fit somewhere between LaTeX and markdow. I made this tool to speed up note taking for my upcoming physics degree (starting september 2026).

## Early version Disclaimer

This tool was designed for a personnal use, and it is in an early version. Hence you should not expect proper error handling, rigorous testing or fancy features and adaptability. I may or may not add them in the future, but if you want feel free to contribute (just please ask before adding features). All contributions are highly appreciated 😀.

## Command line arguments
Work in progress.

## The syntax
### Front matter
Work in progress. It will define document metadata like title, author and date.

### Environments
I use a block system for environments. For now it doesn't support nested blocks (not tested so undefined behaviour) as they are intended for content organization and not math block.

```
@<environment> <optional text>
<content>
@@
```

Example :

```
@theorem Théorème de Pythagore
Dans un triangle ABC rectangle en A, $$AB^2 + AC^2 = BC^2$$
@@
```
There are no defined list of environment but if you use one that isn't defined your LaTeX compiler will most likely produce an error.

### Headers
Headers work just like they would in markdown. Just type between one and three '#' at the start of a line followed by the text. Headers don't work inside environments.

```
# Header 1 will become a section
## Header 2 will become a subsection
### Header 3 will become a subsubsection
```

Headers with more than three '#' will be treated as a header 3.

### Formatting
For now only bold formatting, like in markdown (**`**bold text**`**) is supported, italic and underlining are coming soon (or later).

### Lists
Lists should work like in markdown.

```
- Si et seulement si $\Re(z) = 0$, alors $z$ est imaginaire pur ($z \in \iR$).
- Si et seulement si $\Im(z) = 0$, alors $z$ est réel ($z \in \R$).
```

## AI Disclaimer

Most of this software was hand-written by me, expect a few parts (in the parser I think) I made using AI because I didn't want to dedicate too much time to have a first version.

Also you can probably tell by the very approximative english level but this README is human made.
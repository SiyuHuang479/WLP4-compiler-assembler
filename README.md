# WLP4 Compiler

A compiler written in C++ that translates WLP4, a small C-like language, into MIPS assembly. It implements four stages:

```text
WLP4 source -> Tokens -> Parse tree -> Typed parse tree -> MIPS assembly
```

## Project Files

| File | Purpose |
| --- | --- |
| `wlp4scan.cc` | Splits source code into tokens such as keywords, identifiers, and numbers. |
| `wlp4parse.cc` | Checks the token sequence against the grammar and builds a parse tree. |
| `wlp4type.cc` | Checks declarations and types, then annotates the parse tree. |
| `wlp4gen.cc` | Converts the typed tree into MIPS assembly. |

The implementation includes integer and pointer expressions, variable declarations, assignments, conditionals, loops, procedure calls, `println`, and dynamic memory allocation with `new` and `delete`.


## Build


```bash
c++ -std=c++14 -o wlp4scan wlp4scan.cc
c++ -std=c++14 -o wlp4parse wlp4parse.cc
c++ -std=c++14 -o wlp4type wlp4type.cc
c++ -std=c++14 -o wlp4gen wlp4gen.cc
```

## Usage

Save a WLP4 program as `example.wlp4`, for example:

```c
int wain(int a, int b) {
    return a + b;
}
```

Each stage reads standard input and writes standard output. Run the stages in order:

```bash
./wlp4scan < example.wlp4 > example.tokens
./wlp4parse < example.tokens > example.tree
./wlp4type < example.tree > example.typed
./wlp4gen < example.typed > example.asm
```

Check each stage for errors before continuing. The final file, `example.asm`, is assembly text, not a runnable executable.

## Current Status

The scanner, type checker, and code generator pass C++14 syntax checks. The parser cannot compile without `wlp4data.h`, and the full pipeline has not been verified because the support files are missing. No automated tests are included in this checkout.

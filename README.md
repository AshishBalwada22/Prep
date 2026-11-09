# Prep

C++ interview internals, written as code you can run.

Every file here is built around a trap: a program that looks obviously correct, an output prediction you should make before compiling, and then the explanation of why the compiler or the runtime did the surprising thing. Reading these is useful. Predicting them first is what actually prepares you.

## The files

| File | The trap it covers |
|---|---|
| `1_constructor_ordering_and_padding.cpp` | construction and destruction order, `vptr` initialisation order, struct padding and what `sizeof` really counts |
| `2_copy_constructor_vs_assignment.cpp` | deep vs shallow copy, self-assignment, why the two operators are not interchangeable |
| `3_RAII_file_handling.cpp` | RAII applied to file handles, plus bug-finding on a version that leaks them |
| `4_pointer_reference_const.cpp` | the full const-correctness matrix for pointers and references |
| `5_virtual_pointer_table.cpp` | `vptr`/`vtable`, virtual vs non-virtual dispatch, and the cases where the "obvious" output is wrong |
| `6_modern_c++_RAII_sementic.cpp` | RAII semantics in modern C++, ownership as a type property |
| `7_copy_constructor_move_constructor.cpp` | the rule of five, and what actually happens on each operation |
| `8_move_sementic_return_value_optimization.cpp` | move semantics vs return value optimisation — when a move is elided entirely |
| `9_STL.cpp` | iterator invalidation, the class of bug that compiles, runs, and corrupts silently |
| `10_threading.cpp` | `std::thread` basics and the ordering problems that follow |
| `11_undefined_behavior.cpp` | undefined behaviour that compiles cleanly and then lies to you |
| `C++_tricky_question.cpp` | ~1,100-line bank of "what does this print?" questions, accumulating everything above |

## How to use it

```bash
g++ -std=c++17 -Wall -pthread 5_virtual_pointer_table.cpp -o vtable && ./vtable
```

Read the question at the top of a file, write down your predicted output, then compile. When the answer differs from your prediction, that gap is the thing worth remembering.

Some files are deliberately broken — the UB and bug-finding ones are meant to be compiled with `-fsanitize=address,undefined` to see the failure the standard allows.

```bash
g++ -std=c++17 -g -fsanitize=address,undefined 11_undefined_behavior.cpp -o ub && ./ub
```

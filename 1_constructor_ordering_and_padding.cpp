What is the output of this program?

#include <iostream>
using namespace std;
struct Foo {
    Foo()  { cout << "Foo()\n"; }
    ~Foo() { cout << "~Foo()\n"; }
};
struct Bar {
    Foo f;
    Bar() { cout << "Bar()\n"; }
    ~Bar() { cout << "~Bar()\n"; }
};
int main() {
    Bar b;
    return 0;
}



Output - 
Foo()
Bar()
~Bar()
~Foo()


Imp - Member subobjects are initialized before the constructor body executes, in declaration order. 
Destruction is strictly reverse of construction.


------------------------------------------------------------------------------------------
How much memory does this struct occupy? Explain why.

struct A {
    char  a;    // 1 byte
    int   b;    // 4 bytes
    char  c;    // 1 byte
};
struct B {
    char  a;    // 1 byte
    char  c;    // 1 byte
    int   b;    // 4 bytes
};

What is sizeof(A) and sizeof(B)?

Imp - 
Every member must be stored at an address that is a multiple of its own size. 
The struct's total size must be a multiple of its largest member's size.

struct A {
    char  a;    // 1 byte
    int   b;    // 4 bytes
    char  c;    // 1 byte
};


Offset 0:  [a]          ← char fits anywhere
Offset 1:  [pad][pad][pad]  ← int needs offset % 4 == 0
                              so next valid offset = 4
Offset 4:  [b][b][b][b] ← int occupies bytes 4,5,6,7
Offset 8:  [c]          ← char fits
Offset 9:  [pad][pad][pad]  ← struct size must be multiple of 4
                              so padded to 12

sizeof(A) = 12 byte

struct B {
    char  a;    // 1 byte
    char  c;    // 1 byte
    int   b;    // 4 bytes
};

Offset 0:  [a]          ← char
Offset 1:  [c]          ← char, fits right after
Offset 2:  [pad][pad]   ← int needs offset % 4 == 0
                          next valid = 4
Offset 4:  [b][b][b][b] ← int


sizeof(B) = 8 byte

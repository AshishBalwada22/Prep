Modern C++

lvalue    → has address, lives beyond statement (variables, objects)
rvalue    → temporary, no address, dies at statement end (literals, temps)
move      → steal resources from rvalue instead of copying
unique_ptr → sole owner, cannot copy, can move
shared_ptr → reference counted owner, can copy




lvalue vs rvalue — Identify Each

Classify each as lvalue or rvalue:

int x = 10;           // A: x is ___
int& ref = x;         // B: x is ___
int getNum() { return 42; }
int y = getNum();     // C: getNum() is ___
int&& rref = 42;      // D: 42 is ___
int z = x + 5;        // E: x + 5 is ___
int* p = &x;          // F: &x is ___
int* q = &(x + 5);    // G: &(x + 5) is ___  — does this compile?
For each, answer: lvalue or rvalue? And for G, does it compile?


A: x is lvalue             (variable, has address)
B: x is lvalue             (ref doesn't change that x is lvalue)
C: getNum() is rvalue      (temporary, dies after statement)
D: 42 is rvalue            (literal, temporary)
E: x + 5 is rvalue         (expression result, temporary)
F: &x is lvalue            (address-of returns lvalue)
G: &(x + 5) won't compile  (cannot take address of rvalue)

int* q = &(x + 5);   // error
Why?
x + 5 is rvalue (temporary expression result)
Temporary has no persistent address
It lives only for this statement
Compiler forbids taking address of temporary
Because pointer would dangle immediately


Timeline:
─────────────────────────────────
x + 5 evaluated → result = 15
&(x + 5) tries to get address
15 is temporary, no storage location
statement ends → 15 dies
pointer would point to dead memory
→ Compiler says NO




But There's A Workaround

int* q = &(x + 5);           // ❌ error — rvalue to non-const lvalue ref
const int* q = &(x + 5);     // ✅ compiles! — rvalue to const lvalue ref
int&& rref = x + 5;          // ✅ compiles! — rvalue to rvalue ref

Why does const int* work?

const int* q = &(x + 5);
→ binding rvalue to const reference
→ compiler creates temporary storage
→ q points to that temporary
→ temporary lives as long as const ref lives
→ safe!
But non-const ref:
int* q = &(x + 5);
→ non-const pointer could modify temporary
→ dangerous, forbidden


Summary — lvalue vs rvalue
lvalue:
✅ has persistent address
✅ lives beyond current statement
✅ can bind to lvalue reference (int& r = x)
✅ can take address (&x)
rvalue:
✅ temporary, no persistent address
✅ dies at end of statement
✅ can bind to rvalue reference (int&& r = 42)
✅ can bind to const lvalue reference (const int& r = 42)
❌ cannot take address (&(x+5) is error)
❌ cannot bind to non-const lvalue reference


Reference Binding — Three Types
A reference is an alias to a variable. 

1) lvalue reference: `int& r = x`
int x = 10;
int& r = x;    // ✅ binds to lvalue
What it means:

r is now another name for x
r and x point to same memory
Modifying r modifies x



What can bind to int&:

int x = 10;
int& r1 = x;           // ✅ lvalue variable
int& r2 = 42;          // ❌ error — cannot bind rvalue to non-const lvalue ref
int& r3 = x + 5;       // ❌ error — cannot bind rvalue expression to non-const lvalue ref


Why the restriction?

If you could do: int& r = 42;
Then r would be alias to temporary 42
Temporary dies at end of statement
r becomes dangling reference
→ Compiler forbids this


2)  rvalue reference: `int&& r = 42`
int&& r = 42;    // ✅ binds to rvalue

What it means:
r is a reference to a temporary
The temporary is kept alive as long as r exists
You can modify the temporary through r


int&& r = 42;
r = 99;
cout << r;     // 99 — temporary was modified



What can bind to int&&:

int&& r1 = 42;          // ✅ rvalue literal
int&& r2 = x + 5;       // ✅ rvalue expression
int&& r3 = x;           // ❌ error — cannot bind lvalue to rvalue ref
int&& r4 = move(x);     // ✅ move() converts lvalue to rvalue


3)  const lvalue reference: `const int& r = 42`

const int& r = 42;    // ✅ binds to rvalue!

What it means:
r is a const reference to a temporary
Temporary is kept alive as long as r exists
You cannot modify through r (it's const)


const int& r = 42;
r = 99;              // ❌ error — r is const
cout << r;           // 42 — still original value


What can bind to const int&:

int x = 10;
const int& r1 = x;          // ✅ lvalue
const int& r2 = 42;         // ✅ rvalue
const int& r3 = x + 5;      // ✅ rvalue expression

Why does const int& accept rvalue?

const int& r = 42;
→ compiler creates temporary storage
→ stores 42 there
→ r binds to that temporary
→ temporary lives as long as r lives
→ safe because r is const (cannot modify)



Real World Analogy
int& r = x;
→ giving someone your house key
→ they can visit and modify your house
→ must be YOUR house (lvalue)
int&& r = 42;
→ temporary hotel room (rvalue)
→ you get the key to the room
→ you can modify the room
→ room exists only while you're there
const int& r = 42;
→ temporary museum (rvalue)
→ you get a tour pass
→ you can look but cannot modify
→ museum exists while you're visiting


Key Takeaways
int& r = x;
→ non-const lvalue reference
→ accepts ONLY lvalues
→ used for: output parameters, modifying original
int&& r = 42;
→ rvalue reference
→ accepts ONLY rvalues
→ used for: move semantics, perfect forwarding
const int& r = 42;
→ const lvalue reference
→ accepts BOTH lvalues and rvalues
→ used for: function parameters (accept anything)
→ cannot modify through reference
1) Copy Constructor vs Assignment Operator

#include <iostream>
#include <cstring>
using namespace std;

class MyString {
    char* data;
public:
    MyString(const char* str) {
        data = new char[strlen(str) + 1];
        strcpy(data, str);
        cout << "Constructor\n";
    }

    MyString(const MyString& other) {
        data = other.data;
        cout << "Copy Constructor\n";
    }

    MyString& operator=(const MyString& other) {
        data = other.data;
        cout << "Assignment\n";
        return *this;
    }

    ~MyString() {
        cout << "Destructor\n";
        delete[] data;
    }
};

int main() {
    MyString a("Hello");
    MyString b = a;       // Line A
    MyString c("World");
    c = a;                // Line B
    return 0;
}


If the object is being initialized → Copy Constructor
If the object already exists → Assignment Operator

MyString b = a;   // b doesn't exist yet → INITIALIZATION → Copy Constructor
c = a;            // c already exists   → ASSIGNMENT     → operator=

Critical Bug — Shallow Copy
data = other.data;  // ← Just copying the POINTER, not the data

 Proper Fix — Deep Copy
 MyString(const MyString& other) {
    // Step 1: allocate NEW memory
    data = new char[strlen(other.data) + 1];
    // Step 2: copy the actual DATA, not the pointer
    strcpy(data, other.data);
    cout << "Copy Constructor\n";
}

Fix Assignment Operator 
MyString& operator=(const MyString& other) {
    // Step 1: self-assignment check ← CRITICAL, most candidates forget this
    if (this == &other) return *this;

    // Step 2: free existing memory ← CRITICAL, most candidates forget this
    delete[] data;

    // Step 3: deep copy
    data = new char[strlen(other.data) + 1];
    strcpy(data, other.data);

    cout << "Assignment\n";
    return *this;
}

Rule of 3
class MyString {
    // Rule of 3: all three must be properly defined
    ~MyString();                              // 1 Destructor
    MyString(const MyString&);               // 2 Copy Constructor
    MyString& operator=(const MyString&);    // 3 Copy Assignment Operator
};

"Line A calls copy constructor because b is being initialized. Line B calls assignment because c already exists.
 The bug is shallow copy — both objects share the same pointer. At destruction, double delete causes undefined behavior. Fix: deep copy in both copy constructor and assignment.
 Assignment also needs a self-assignment guard and must free old memory first. This is the Rule of 3."


 -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

 2) RAII + Destructor Trap

 #include <iostream>
using namespace std;

class Resource {
    int* data;
public:
    Resource(int val) {
        data = new int(val);
        cout << "Resource acquired: " << *data << "\n";
    }

    ~Resource() {
        cout << "Resource released: " << *data << "\n";
        delete data;
    }

    int getValue() { return *data; }
};

void process() {
    Resource r1(10);
    Resource r2(20);

    if (true) {
        Resource r3(30);
        throw runtime_error("Something went wrong");
    }

    cout << "This line never executes\n";
}

int main() {
    process();
    return 0;
}

What is the exact output of this program?
Are the resources properly released even though an exception is thrown?
What fundamental C++ guarantee makes this work — or not work?
What is RAII and why does it matter here?
What happens if we used raw new instead of wrapping in a class — how would the behavior change?

1) Actual output:

Resource acquired: 10
Resource acquired: 20
Resource acquired: 30
Resource released: 30
Resource released: 20
Resource released: 10

2) Are Resources Properly Released?
Yes, When an exception is thrown, C++ guarantees that all fully constructed objects on the stack will have their destructors called during stack unwinding.
stack unwinding
Exception flow:
r1 created → r2 created → r3 created → EXCEPTION!
                                            ↓
                              C++ runtime says:
                              "I need to clean up this scope
                               even though } was never reached"
                                            ↓
                              Forcefully calls destructors
                              in reverse order

3) Stack Unwinding ->  C++ guarantees that destructors of all fully constructed local objects are called as the stack is unwound.
4) RAII = Resource Acquisition Is Initialization
The core idea:

Acquire resource  →  in constructor
Release resource  →  in destructor

5) void process() {
    int* r1 = new int(10);
    int* r2 = new int(20);
    int* r3 = new int(30);

    throw runtime_error("Something went wrong");

    delete r3;  // ← NEVER REACHED
    delete r2;  // ← NEVER REACHED
    delete r1;  // ← NEVER REACHED
}

Exception thrown
     ↓
Stack unwinds — but raw pointers have NO destructors
     ↓
delete statements never execute
     ↓
r1, r2, r3 memory → LEAKED FOREVER 💥

RAII     =  Resource lifetime tied to object lifetime
Unwinding =  Destructors called in LIFO on exception

------------------------------------------------------------------------------------------------------------------------------------------------------------
3) Virtual Functions + vtable

#include <iostream>
using namespace std;

class Animal {
public:
    Animal() {
        cout << "Animal Constructor\n";
        speak();  // ← Line X
    }

    ~Animal() {
        cout << "Animal Destructor\n";
        speak();  // ← Line Y
    }

    virtual void speak() {
        cout << "Animal speaks\n";
    }
};

class Dog : public Animal {
public:
    Dog() {
        cout << "Dog Constructor\n";
        speak();  // ← Line Z
    }

    ~Dog() {
        cout << "Dog Destructor\n";
    }

    void speak() override {
        cout << "Dog barks\n";
    }
};

int main() {
    Animal* ptr = new Dog();
    cout << "---\n";
    delete ptr;
    return 0;
}


What is the exact output of this program — line by line?
What does Line X print — "Animal speaks" or "Dog barks"? Why?
What does Line Y print — "Animal speaks" or "Dog barks"? Why?
What is a vtable and how does virtual dispatch work?
What is the critical bug in this code unrelated to virtual functions?
How do you fix that bug?

1)  Question 1 — Exact Output
Animal Constructor        ← Animal constructor runs first
Animal speaks             ← Line X (THIS IS THE TRAP)
Dog Constructor           ← Dog constructor runs second
Dog barks                 ← Line Z
---
Dog Destructor            ← Dog destructor runs first
Animal Destructor         ← Animal destructor runs second
Animal speaks             ← Line Y (TRAP AGAIN)

Rule - Base class constructor ALWAYS runs before derived class constructor

2) Question 2 — Line X — THE CORE TRAP
Animal constructor is running
↓
At this moment, Dog object does NOT exist yet
↓
Dog's speak() belongs to Dog
↓
But Dog hasn't been constructed yet
↓
C++ cannot safely call Dog::speak()
↓
Therefore: Animal::speak() is called instead

[Animal constructor running]
    → vtable pointer points to Animal's vtable
    → speak() → Animal::speak() → "Animal speaks"
    
[Dog constructor running]  
    → vtable pointer NOW updated to Dog's vtable
    → speak() → Dog::speak() → "Dog barks"

3) Line Y — Same Trap In Reverse
Actual output: "Animal speaks" ✅
Destruction order: Dog destructor → Animal destructor

[Dog destructor running]
    → Dog object being dismantled
    → vtable pointer DOWNGRADED back to Animal's vtable
    
[Animal destructor running]
    → speak() → Animal::speak() → "Animal speaks"
During Animal's destructor, Dog's part is already destroyed. Calling Dog::speak() would be undefined behavior. C++ protects you by reverting the vtable.

4) vtable Explained Properly
Every class with virtual functions has a vtable:
Animal's vtable:
┌─────────────────────────────────┐
│  speak  →  Animal::speak()      │
└─────────────────────────────────┘

Dog's vtable:
┌─────────────────────────────────┐
│  speak  →  Dog::speak()         │
└─────────────────────────────────┘
Every object has a hidden vptr:
Dog object in memory:
┌──────────────────────────────────┐
│  vptr  →  points to Dog's vtable │  ← hidden pointer added by compiler
│  (Animal data members)           │
│  (Dog data members)              │
└──────────────────────────────────┘

How virtual dispatch works:

    
Animal* ptr = new Dog();
ptr->speak();
ⓘ
For code that is intended to be used in Siemens products or services, the code generation features of our AI Services may only be used after prior approval of your responsible organizational unit.
1. Compiler sees: ptr is Animal* and speak() is virtual
2. At runtime: follow ptr → find object → find vptr
3. vptr → Dog's vtable → Dog::speak()
4. Call Dog::speak() → "Dog barks"
This is runtime polymorphism — the decision of which function to call happens at runtime, not compile time.

5) The REAL Critical Bug
    
Animal* ptr = new Dog();
delete ptr;              // ← BUG HERE

Why?
delete ptr
↓
ptr is of type Animal*
↓
Compiler looks up destructor
↓
~Animal() is NOT virtual
↓
Static dispatch → only ~Animal() called
↓
~Dog() never runs → memory leak

6)  The Real Fix

class Animal {
public:
    virtual ~Animal() {        // ← add virtual keyword here
        cout << "Animal Destructor\n";
        speak();
    }
};

delete ptr
↓
ptr is Animal* but destructor is virtual
↓
Runtime checks vtable
↓
Finds Dog's destructor
↓
~Dog() called first ✅
~Animal() called second ✅
Both destructors run → no leak ✅

--------------------------------------------------------------------------------------------------------------------------------------------------------

5) Templates + Type Deduction Trap
#include <iostream>
#include <typeinfo>
using namespace std;

template <typename T>
void printType(T val) {
    cout << "Value: " << val << "\n";
    cout << "Type: " << typeid(T).name() << "\n\n";
}

template <typename T>
void printRef(T& val) {
    cout << "Value: " << val << "\n";
    cout << "Type: " << typeid(T).name() << "\n\n";
}

template <typename T>
void printConstRef(const T& val) {
    cout << "Value: " << val << "\n";
    cout << "Type: " << typeid(T).name() << "\n\n";
}

int main() {
    int x = 42;
    const int cx = 100;
    int& rx = x;

    cout << "--- printType ---\n";
    printType(x);       // Call 1
    printType(cx);      // Call 2
    printType(rx);      // Call 3

    cout << "--- printRef ---\n";
    printRef(x);        // Call 4
    printRef(cx);       // Call 5

    cout << "--- printConstRef ---\n";
    printConstRef(x);   // Call 6
    printConstRef(cx);  // Call 7

    return 0;
}

For each of the 7 calls, what does T get deduced as?
What happens to const when passed to printType(T val)? Why?
What happens to references when passed to printType(T val)? Why?
What is the difference between T, T&, and const T& in template deduction?
Why does Call 5 printRef(cx) potentially cause a compilation issue — or does it? Explain.
What is type decay and when does it happen?


template <typename T>
void print(T val) {
    cout << val << "\n";
}

T is a placeholder for any type. The compiler fills it in automatically.

print(42);        // T = int
print(3.14);      // T = double
print("hello");   // T = const char*

3 Ways To Receive a Template Argument
template <typename T>  void byValue    (T val)        // Way 1
template <typename T>  void byRef      (T& val)       // Way 2
template <typename T>  void byConstRef (const T& val) // Way 3

Way 1 — T val (Pass By Value)
template <typename T>
void printType(T val) { ... }


Rule: TWO things are always stripped away:
const  → stripped ❌
&      → stripped ❌


template <typename T>
void printType(T val) {
    cout << "Value: " << val << "\n";
    cout << "Type: " << typeid(T).name() << "\n\n";
}

    
int x = 42;
const int cx = 100;
int& rx = x;

printType(x);    // T = int          (plain int)
printType(cx);   // T = int          (const stripped ✅)
printType(rx);   // T = int          (& stripped ✅)



Way 2 — T& (Pass By Reference)

Rule: Reference is kept, const IS preserved
const  → KEPT ✅
&      → stripped (already a reference)

template <typename T>
void printRef(T& val) { ... }

int x = 42;
const int cx = 100;

printRef(x);    // T = int         → val is int&
printRef(cx);   // T = const int   → val is const int&

Way 3 — const T& (Pass By Const Reference)

Rule: const is already in the signature — T is always non-const
const  → stripped from T (already in signature)
&      → stripped from T (already in signature)

template <typename T>
void printConstRef(const T& val) { ... }

int x = 42;
const int cx = 100;

printConstRef(x);    // T = int    → val is const int&
printConstRef(cx);   // T = int    → val is const int&

What Is Type Decay?
Type decay = when a type loses its const, &, or array/function properties during template deduction.

The 3 Rules — Memorize These
┌─────────────────────────────────────────────────────────┐
│ Rule 1: T val                                           │
│   → const stripped, & stripped                          │
│   → you get a fresh copy, qualifiers don't matter       │
│                                                         │
│ Rule 2: T& val                                          │
│   → const KEPT, & stripped                              │
│   → you see the original, const must be respected       │
│                                                         │
│ Rule 3: const T& val                                    │
│   → const stripped from T, already in signature         │
│   → T is always the base type                           │
└─────────────────────────────────────────────────────────┘
Quick Mental Trick
Ask yourself:

"Am I making a copy or looking at the original?"

Copy (T val)        → strip everything, fresh start
Original (T&)       → keep const, it belongs to original
Const ref (const T&)→ T is always base type, const is in signature


-------------------------------------------------------------------------------------------------------------------------------------------------------

6) Move Semantics + Rvalue References


#include <iostream>
#include <string>
using namespace std;

class Buffer {
public:
    int* data;
    int size;

    // Constructor
    Buffer(int s) : size(s), data(new int[s]) {
        cout << "Constructor: allocated " << size << "\n";
    }

    // Copy Constructor
    Buffer(const Buffer& other) : size(other.size), data(new int[other.size]) {
        copy(other.data, other.data + other.size, data);
        cout << "Copy Constructor: copied " << size << "\n";
    }

    // Move Constructor
    Buffer(Buffer&& other) : size(other.size), data(other.data) {
        other.data = nullptr;
        other.size = 0;
        cout << "Move Constructor: moved " << size << "\n";
    }

    // Copy Assignment
    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            delete[] data;
            size = other.size;
            data = new int[size];
            copy(other.data, other.data + size, data);
            cout << "Copy Assignment: copied " << size << "\n";
        }
        return *this;
    }

    // Move Assignment
    Buffer& operator=(Buffer&& other) {
        if (this != &other) {
            delete[] data;
            size = other.size;
            data = other.data;
            other.data = nullptr;
            other.size = 0;
            cout << "Move Assignment: moved " << size << "\n";
        }
        return *this;
    }

    ~Buffer() {
        delete[] data;
        cout << "Destructor: size " << size << "\n";
    }
};

Buffer createBuffer() {
    Buffer temp(5);
    return temp;
}

int main() {
    cout << "--- 1 ---\n";
    Buffer b1(10);

    cout << "--- 2 ---\n";
    Buffer b2 = b1;

    cout << "--- 3 ---\n";
    Buffer b3 = move(b1);

    cout << "--- 4 ---\n";
    Buffer b4 = createBuffer();

    cout << "--- 5 ---\n";
    b2 = move(b3);

    cout << "--- END ---\n";
    return 0;
}

What is an lvalue and what is an rvalue? Give examples from this code
What does && mean in Buffer(Buffer&& other)?
What is the difference between copy and move? Why is move faster?
What does move(b1) actually do?
What happens to b1 after Buffer b3 = move(b1)? Is it safe to use?
What is RVO/NRVO and how does it affect createBuffer()?
What is the exact output of this program — section by section?
After b2 = move(b3) — what is the state of b3?


class Buffer {
public:
    int* data;   // pointer to heap memory
    int size;
};


    
Buffer b1(1000000);   // allocate 1 million ints
Buffer b2 = b1;       // what happens here?

b1:  data → [1, 2, 3, 4, ... 1000000 ints on heap]

Copy means:
→ allocate NEW memory for b2
→ copy ALL 1 million values one by one
→ b2: data → [1, 2, 3, 4, ... 1000000 ints on heap]

Both b1 and b2 exist with their own memory
This is EXPENSIVE 💸
Question: What if you don't need b1 anymore after copying? You just wasted time copying 1 million values for no reason.


lvalue vs rvalue — Foundation of Move Semantics

Simple Rule:
lvalue = has a name, has a permanent address, lives long
rvalue = no name, temporary, dies immediately after expression

int x = 42;
//  ↑         ↑
// lvalue    rvalue (42 is temporary, has no name)

Buffer b1(10);
//     ↑
//   lvalue (b1 has a name, lives until end of scope)

Buffer b2 = createBuffer();
//          ↑
//        rvalue (temporary object returned from function)

b2 = move(b1);
//   ↑
// rvalue (move() casts b1 to rvalue)


Can you take its address with & ?
&b1   → works ✅ → b1 is lvalue
&42   → fails ❌ → 42 is rvalue
&createBuffer() → fails ❌ → temporary is rvalue


lvalue Reference vs rvalue Reference
int x = 42;

int&  lref = x;    // lvalue reference  → binds to lvalue
int&& rref = 42;   // rvalue reference  → binds to rvalue

&   = lvalue reference  = "I refer to something permanent"
&&  = rvalue reference  = "I refer to something temporary"

The Key Insight:
If something is temporary (rvalue)
→ it is going to die anyway
→ we can STEAL its resources instead of copying
→ this is MOVE semantics



Copy vs Move


Copy Constructor:
Buffer(const Buffer& other) {
    // other is lvalue reference
    // other still needs its data after this
    // so we COPY everything

    data = new int[other.size];           // allocate new memory
    copy(other.data, other.data + size, data);  // copy all values
}
Before:  other.data → [1, 2, 3, 4, 5]
After:   other.data → [1, 2, 3, 4, 5]   ← other unchanged
         this.data  → [1, 2, 3, 4, 5]   ← new copy

Move Constructor:
Buffer(Buffer&& other) {
    // other is rvalue reference
    // other is TEMPORARY — it will die anyway
    // so we STEAL its pointer instead of copying

    data = other.data;      // steal the pointer
    size = other.size;      // steal the size
    other.data = nullptr;   // leave other empty
    other.size = 0;         // leave other empty
}

Before:  other.data → [1, 2, 3, 4, 5]

After:   other.data → nullptr            ← other is emptied
         this.data  → [1, 2, 3, 4, 5]   ← same memory, no cop



What Does std::move() Actually Do?
Buffer b3 = move(b1);
std::move() does NOT move anything.
std::move() simply CASTS an lvalue to an rvalue reference

    
// move() is essentially this:
template<typename T>
T&& move(T& val) {
    return static_cast<T&&>(val);  // just a cast
}


b1 is lvalue
move(b1) → casts b1 to rvalue reference
Now compiler sees rvalue → calls Move Constructor instead of Copy Constructor
Move Constructor actually does the moving

Timeline:
Buffer b3 = move(b1);
              ↓
         move(b1) casts b1 to rvalue
              ↓
         Compiler sees rvalue argument
              ↓
         Calls Buffer(Buffer&& other)   ← Move Constructor
              ↓
         Steals b1's data pointer
              ↓
         b1.data = nullptr, b1.size = 0
              ↓
         b3 now owns the memory



State After Move 
Buffer b3 = move(b1);
// Now what is b1?
b1.data = nullptr
b1.size = 0


RVO / NRVO — Return Value Optimization / Named Return Value Optimization

Buffer createBuffer() {
    Buffer temp(5);
    return temp;       // returning local variable
}

Buffer b4 = createBuffer();

Without Optimization — What You'd Expect:
1. temp constructed inside createBuffer()
2. temp copied/moved to return value
3. return value copied/moved to b4
4. temp destroyed
5. return value destroyed
= 3 constructions, 2 destructions (expensive)
With NRVO — What Actually Happens:
Compiler is smart:
→ It sees temp will be returned
→ It constructs temp DIRECTLY in b4's memory location
→ No copy, no move needed at all
= 1 construction only ✅ (free optimization)
NRVO = Named Return Value Optimization
RVO  = Return Value Optimization (for unnamed temporaries)


Golden Rules — Memorize These
1. lvalue  = has name = use copy
2. rvalue  = temporary = use move
3. move()  = just a cast, not actual moving
4. After move: object is valid but empty
5. Move is O(1), Copy is O(N)
6. RVO eliminates copies on return
7. Always set moved-from pointer to nullptr


---------------------------------------------------------------------------------------------

Smart Pointers + Ownership Model

#include <iostream>
#include <memory>
using namespace std;

class Resource {
public:
    string name;
    Resource(string n) : name(n) {
        cout << "Resource created: " << name << "\n";
    }
    ~Resource() {
        cout << "Resource destroyed: " << name << "\n";
    }
    void use() {
        cout << "Using: " << name << "\n";
    }
};

void takeUnique(unique_ptr<Resource> ptr) {
    ptr->use();
}

void peekUnique(const unique_ptr<Resource>& ptr) {
    ptr->use();
}

shared_ptr<Resource> createShared() {
    shared_ptr<Resource> s = make_shared<Resource>("SharedOne");
    return s;
}

int main() {
    cout << "--- 1: unique_ptr ---\n";
    unique_ptr<Resource> u1 = make_unique<Resource>("Alpha");
    unique_ptr<Resource> u2 = move(u1);

    cout << "--- 2: use after move ---\n";
    if (u1) {
        u1->use();
    } else {
        cout << "u1 is empty\n";
    }

    cout << "--- 3: pass to function ---\n";
    peekUnique(u2);
    takeUnique(move(u2));

    cout << "--- 4: shared_ptr ---\n";
    shared_ptr<Resource> s1 = createShared();
    cout << "Count: " << s1.use_count() << "\n";
    shared_ptr<Resource> s2 = s1;
    cout << "Count: " << s1.use_count() << "\n";
    {
        shared_ptr<Resource> s3 = s1;
        cout << "Count: " << s1.use_count() << "\n";
    }
    cout << "Count: " << s1.use_count() << "\n";

    cout << "--- 5: weak_ptr ---\n";
    weak_ptr<Resource> w1 = s1;
    cout << "Count: " << s1.use_count() << "\n";
    {
        shared_ptr<Resource> locked = w1.lock();
        if (locked) {
            locked->use();
            cout << "Count: " << s1.use_count() << "\n";
        }
    }
    cout << "Count: " << s1.use_count() << "\n";

    cout << "--- END ---\n";
    return 0;
}


Tell me:

What is a smart pointer and why do we need it?
What is unique_ptr — what are its ownership rules?
Why does unique_ptr<Resource> u2 = u1 fail to compile but u2 = move(u1) works?
What is shared_ptr — how does reference counting work?
What is weak_ptr — why does it exist and what problem does it solve?
What is the circular reference problem — give an example
What is the exact output of this program section by section?
After takeUnique(move(u2)) — when is "Alpha" resource destroyed?


1) Why Do We Need Smart Pointers?

The Old Way — Raw Pointers (Dangerous):

void oldWay() {
    Resource* ptr = new Resource("Alpha");  // allocate
    
    // ... lots of code ...
    
    if (something_failed) {
        return;          // ❌ forgot to delete! MEMORY LEAK
    }
    
    delete ptr;          // must remember to delete manually
}

Problems With Raw Pointers:
❌ Forget to delete    → memory leak
❌ Delete twice        → crash (double free)
❌ Use after delete    → undefined behavior
❌ Exception thrown    → delete never reached → leak


Smart Pointers Fix All Of This:

void newWay() {
    unique_ptr<Resource> ptr = make_unique<Resource>("Alpha");
    
    // ... lots of code ...
    
    if (something_failed) {
        return;    // ✅ ptr destroyed automatically here
    }
    
}   // ✅ ptr destroyed automatically here too
    // No manual delete needed EVER


Smart pointers are objects that wrap a raw pointer and automatically delete it when they go out of scope. This is called RAII.

RAII = Resource Acquisition Is Initialization
     = when object dies → resource is cleaned up automatically



2) The 3 Smart Pointers

} unique_ptr   │ Only ONE owner at a time               │
│ shared_ptr   │ MULTIPLE owners, reference counted     │
│ weak_ptr     │ Observer, does NOT own the resource    │



3) unique_ptr — Single Ownership

Core Idea:
unique_ptr = "I am the ONLY owner of this resource"
→ When I die, the resource dies with me
→ Nobody else can own it
→ You can TRANSFER ownership but not SHARE it


unique_ptr<Resource> u1 = make_unique<Resource>("Alpha");
//                        ↑
//                  always use make_unique
//                  never use: unique_ptr<Resource>(new Resource("Alpha"))


Why Can't You Copy It?
unique_ptr<Resource> u2 = u1;    // ❌ COMPILE ERROR

If copy was allowed:
→ u1 owns the resource
→ u2 also owns the resource
→ u1 dies → deletes resource
→ u2 dies → deletes resource AGAIN
→ DOUBLE DELETE → CRASH 💥

So copy is DISABLED by design


You Can Only MOVE It:
unique_ptr<Resource> u2 = move(u1);   // ✅ ownership transferred

Before move:
u1 → Resource("Alpha")
u2 → nullptr

After move:
u1 → nullptr              ← u1 gives up ownership
u2 → Resource("Alpha")    ← u2 is now the owner

Checking If unique_ptr Is Empty:

if (u1) {           // ✅ checks if u1 is not nullptr
    u1->use();
} else {
    cout << "u1 is empty\n";
}


4) Passing unique_ptr To Functions

// Way 1: Pass by move → function TAKES ownership
void takeUnique(unique_ptr<Resource> ptr) {
    ptr->use();
}   // ptr dies here → Resource destroyed here

// Way 2: Pass by const reference → function just PEEKS
void peekUnique(const unique_ptr<Resource>& ptr) {
    ptr->use();
}   // ptr reference dies here → Resource NOT destroyed


takeUnique(move(u2));   // u2 gives up ownership
                        // function owns it now
                        // Resource destroyed when function ends

peekUnique(u2);         // u2 keeps ownership
                        // function just borrows it
                        // Resource NOT destroyed

                
5) shared_ptr — Shared Ownership

Core Idea:
shared_ptr = "Multiple people can own this resource"
→ Keeps a COUNT of how many owners exist
→ When count reaches 0 → resource destroyed
→ This count is called REFERENCE COUNT

How Reference Count Works:

shared_ptr<Resource> s1 = make_shared<Resource>("Alpha");
// count = 1

shared_ptr<Resource> s2 = s1;    // copy → count increases
// count = 2

shared_ptr<Resource> s3 = s1;    // copy → count increases
// count = 3

s3 goes out of scope → count decreases
// count = 2

s2 goes out of scope → count decreases
// count = 1

s1 goes out of scope → count decreases
// count = 0 → Resource DESTROYED

Visual:
s1 ──┐
     ├──▶ [Resource "Alpha"] ◀── [ref count: 3]
s2 ──┤
     │
s3 ──┘

s3 dies:
s1 ──┐
     ├──▶ [Resource "Alpha"] ◀── [ref count: 2]
s2 ──┘

s2 dies:
s1 ────▶ [Resource "Alpha"] ◀── [ref count: 1]

s1 dies:
[ref count: 0] → Resource DESTROYED ✅


use_count() — Check Reference Count:

shared_ptr<Resource> s1 = make_shared<Resource>("Alpha");
cout << s1.use_count();    // 1

shared_ptr<Resource> s2 = s1;
cout << s1.use_count();    // 2

shared_ptr<Resource> s3 = s1;
cout << s1.use_count();    // 3


6) weak_ptr — Non-Owning Observer

Core Idea:
weak_ptr = "I can SEE the resource but I don't OWN it"
→ Does NOT increase reference count
→ Cannot access resource directly
→ Must LOCK it first to get a temporary shared_ptr
→ Lock may fail if resource already destroyed


shared_ptr<Resource> s1 = make_shared<Resource>("Alpha");
// count = 1

weak_ptr<Resource> w1 = s1;
// count = STILL 1 ← weak_ptr does NOT increase count

// To use the resource:
shared_ptr<Resource> locked = w1.lock();
// lock() returns shared_ptr if resource alive
// lock() returns nullptr if resource destroyed

if (locked) {
    locked->use();    // safe to use
    // count = 2 while locked exists
}
// locked goes out of scope → count back to 1

7)  The Circular Reference Problem

Why weak_ptr Exists:

struct Node {
    shared_ptr<Node> next;    // ← problem here
    ~Node() { cout << "Node destroyed\n"; }
};

shared_ptr<Node> a = make_shared<Node>();
shared_ptr<Node> b = make_shared<Node>();

a->next = b;    // a holds b
b->next = a;    // b holds a ← CIRCULAR!

a ──▶ [Node A, count=2] ──next──▶ [Node B, count=2]
                ▲                          │
                └──────────next────────────┘

a goes out of scope → count of A = 1 (b->next still holds it)
b goes out of scope → count of B = 1 (a->next still holds it)

count never reaches 0 → NEITHER destroyed → MEMORY LEAK 💸

Fix With weak_ptr:
struct Node {
    weak_ptr<Node> next;    // ✅ weak_ptr breaks the cycle
    ~Node() { cout << "Node destroyed\n"; }
};

a ──▶ [Node A, count=1] ──next(weak)──▶ [Node B, count=1]

a goes out of scope → count of A = 0 → A destroyed ✅
b goes out of scope → count of B = 0 → B destroyed ✅
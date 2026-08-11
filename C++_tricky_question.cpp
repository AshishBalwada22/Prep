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


vtable   → array of function pointers, one per polymorphic class
vptr     → hidden pointer inside every object, points to vtable
virtual  → function resolved at RUNTIME using vptr → vtable
non-virtual → resolved at COMPILE TIME using static type

Every object with a virtual function has a hidden vptr added by compiler. 


Output Prediction — Virtual vs Non-Virtual

#include <iostream>
using namespace std;
class Animal {
public:
    void speak() {
        cout << "Animal speaks\n";
    }
    virtual void sound() {
        cout << "Animal sound\n";
    }
};
class Dog : public Animal {
public:
    void speak() {
        cout << "Dog speaks\n";
    }
    void sound() override {
        cout << "Dog sound\n";
    }
};
int main() {
    Animal* a = new Dog();
    a->speak();    // Line A
    a->sound();    // Line B
    Animal a2 = Dog();
    a2.speak();    // Line C
    a2.sound();    // Line D
    delete a;
}


Animal speaks    ← Line A
Dog sound        ← Line B
Animal speaks    ← Line C
Animal sound     ← Line D

Line A — `a->speak()` → "Animal speaks"
Animal* a = new Dog();
a->speak();    // speak() is NON-VIRTUAL

speak() is non-virtual
→ resolved at COMPILE TIME
→ compiler looks at STATIC TYPE of a
→ static type = Animal*
→ calls Animal::speak()
→ "Animal speaks"
Dog::speak() is IGNORED completely


Line B — `a->sound()` → "Dog sound"
a->sound();    // sound() is VIRTUAL
sound() is virtual
→ resolved at RUNTIME via vptr → vtable
→ compiler looks at DYNAMIC TYPE of a
→ dynamic type = Dog (actual object)
→ calls Dog::sound()
→ "Dog sound"


Line C & D — Object Slicing Trap
Animal a2 = Dog();   // ← THIS is the trap
Dog object created temporarily
Copied into Animal a2
BUT a2 is Animal type — fixed size
Dog's extra parts are SLICED OFF
a2 is now a pure Animal object
vptr points to Animal's vtable

This is called Object Slicing.
Object slicing occurs when a derived object is copied into a base object by value — the derived parts are lost and the vptr resets to base vtable.

Dog object in memory:
┌─────────────────────┐
│ vptr → Dog vtable   │
│ Animal members      │
│ Dog extra members   │  ← these get SLICED OFF
└─────────────────────┘
After Animal a2 = Dog():
┌─────────────────────┐
│ vptr → Animal vtable│  ← vptr reset to Animal
│ Animal members      │  ← only Animal parts copied
└─────────────────────┘
Dog parts are GONE



 vtable — How It Works Internally

 Animal vtable:          Dog vtable:
┌──────────────┐        ┌──────────────┐
│ sound →      │        │ sound →      │
│ Animal::sound│        │ Dog::sound   │
└──────────────┘        └──────────────┘
Animal* a = new Dog():
┌─────────────────────┐
│ vptr ───────────────┼──→ Dog vtable
│ Animal members      │
│ Dog members         │
└─────────────────────┘
a->sound():
1. follow vptr → Dog vtable
2. find sound() entry → Dog::sound
3. call Dog::sound()
→ "Dog sound" ✅

--------------------------------------------------------------------------------------------------------------------------------------------------------

Virtual Destructor — Classic Trap

#include <iostream>
using namespace std;
class Base {
public:
    Base()  { cout << "Base()\n"; }
    ~Base() { cout << "~Base()\n"; }
};
class Derived : public Base {
public:
    Derived()  { cout << "Derived()\n"; }
    ~Derived() { cout << "~Derived()\n"; }
};
int main() {
    Base* b = new Derived();
    delete b;
}
Three questions:

What is the output?
What is the bug?
How do you fix it — and why does the fix work?

Actual Output
Base()
Derived()
~Base()
~Derived() is NEVER called!


Base* b = new Derived();
delete b;

delete b:
→ b is of type Base*
→ ~Base() is NON-VIRTUAL
→ resolved at COMPILE TIME
→ compiler sees Base* → calls Base::~Base()
→ ~Derived() is NEVER called
→ Derived's resources LEAKED


 Construction vs Destruction
 new Derived() triggers:
1. Base()     ← base constructed first
2. Derived()  ← derived constructed second

Destruction WITHOUT virtual — WRONG
delete b (Base*) triggers:
1. ~Base()    ← only this called
2. ~Derived() ← NEVER called ❌

Destruction WITH virtual — CORRECT
delete b (Base*) triggers:
1. ~Derived() ← called first via vtable
2. ~Base()    ← then base destructor



Why This Happens — vtable Explanation

Without virtual destructor:
Base vtable:
┌─────────────────┐
│ ~Base()         │  ← destructor entry
└─────────────────┘
delete b:
→ b is Base*
→ looks up Base vtable
→ calls ~Base() only
→ ~Derived() never reached

With virtual destructor:
Base vtable:              Derived vtable:
┌─────────────────┐       ┌─────────────────┐
│ virtual ~Base() │       │ ~Derived()      │
└─────────────────┘       └─────────────────┘
delete b:
→ b is Base* but vptr → Derived vtable
→ calls ~Derived() first
→ then automatically calls ~Base()
→ complete cleanup ✅


Fix - 
class Base {
public:
    Base()  { cout << "Base()\n"; }
    virtual ~Base() { cout << "~Base()\n"; }  // ← add virtual
};


The Golden Rule
 If a class has ANY virtual function
→ destructor MUST be virtual
If a class is meant to be inherited
→ destructor MUST be virtual



Without a virtual destructor, deleting a derived object through a base pointer only calls the base destructor — the derived destructor is never invoked. 
This causes resource leaks if the derived class owns any resources. The fix is to declare the base destructor virtual — then delete uses the vtable to find and call the correct destructor chain: derived first, then base.
 Any class intended to be used polymorphically must have a virtual destructor.  



--------------------------------------------------------------------------------------------------------------------------------------

Category: Pure Virtual + Abstract Class

#include <iostream>
using namespace std;
class Shape {
public:
    virtual double area() = 0;    // Line A
    virtual void print() {
        cout << "Area = " << area() << "\n";  // Line B
    }
    virtual ~Shape() {}
};
class Circle : public Shape {
    double r;
public:
    Circle(double r) : r(r) {}
    double area() override {
        return 3.14 * r * r;
    }
};
int main() {
    Shape* s = new Circle(5);
    s->print();                   // Line C
    delete s;
    Shape s2;                     // Line D
}
Four questions:

What does = 0 mean on Line A?
What prints at Line C?
Does Line D compile?
Can you call a virtual function from a base class and have it dispatch to derived — like Line B does?


Pure virtual functions make a class abstract — it cannot be instantiated. You can only use pointers or references to abstract classes, pointing to concrete derived objects. 
The derived class must override all pure virtual functions. Virtual dispatch works even when called from non-overridden base functions — the vtable ensures correct resolution.

// So when s->print() is called:
// 1. print() is NOT virtual dispatched (not overridden)
// 2. Uses base class Shape::print()
// 3. Inside print(), area() IS virtual dispatched
// 4. area() goes to Circle::area() via vtable


--------------------------------------------------------------------------------------------------------------------------------------

Category: Multiple Inheritance + vtable Complexity

What is the output? (This is hard — think carefully)

#include <iostream>
using namespace std;
class A {
public:
    virtual void foo() { cout << "A::foo\n"; }
    virtual ~A() {}
};
class B {
public:
    virtual void bar() { cout << "B::bar\n"; }
    virtual ~B() {}
};
class C : public A, public B {
public:
    void foo() override { cout << "C::foo\n"; }
    void bar() override { cout << "C::bar\n"; }
};
int main() {
    C c;
    
    A* a = &c;
    B* b = &c;
    
    a->foo();     // Line A
    b->bar();     // Line B
    
    cout << (void*)&c << "\n";   // Line C — address of c
    cout << (void*)a << "\n";    // Line D — address of a
    cout << (void*)b << "\n";    // Line E — address of b
}
Three questions:

What prints at Line A and B?
What about Lines C, D, E — are the addresses the same or different?
Why would the addresses be different?

1) C::foo
   C::bar

2) 

Memory Layout — Multiple Inheritance
When a class inherits from multiple base classes, the compiler lays them out sequentially in memory:

C object in memory:
┌──────────────────┐
│ A part:          │  offset 0x1000
│  vptr → A vtable │
│  (A members)     │
├──────────────────┤
│ B part:          │  offset 0x1008 (example: 8 bytes after A)
│  vptr → B vtable │
│  (B members)     │
└──────────────────┘


C c;              // &c = 0x1000 (start of A part)
A* a = &c;        // a = 0x1000 (same as &c, A is first base)
B* b = &c;        // b = 0x1008 (adjusted by compiler!)

The compiler automatically adjusts b to point to the B part!

This is called pointer adjustment or base class offset.

Why This Happens
C object has TWO separate vtables:
  - one for A part
  - one for B part
When you do A* a = &c:
  → a points to A's vtable
  → a->foo() uses A's vtable
When you do B* b = &c:
  → compiler adjusts pointer
  → b points to B's vtable
  → b->bar() uses B's vtable
Both vtables have correct virtual functions for C,
but they're at different memory locations.

This is why multiple inheritance is dangerous — the same object has multiple addresses depending on which base pointer you use.

In multiple inheritance, each base class occupies a separate region in the derived object's memory layout. When converting a derived pointer to a base pointer, the compiler automatically adjusts the address to point to that base's region. 
This is called pointer adjustment.
The same object can have different addresses when viewed through different base pointers — the compiler handles this transparently, but it's a source of subtle bugs.


--------------------------------------------------------------------------------------------------------------------------------------

Virtual Function Edge Case

What is the output?

#include <iostream>
using namespace std;
class Base {
public:
    virtual void func() { cout << "Base\n"; }
};
class Derived : public Base {
public:
    void func() override { cout << "Derived\n"; }
};
int main() {
    Base b;
    b.func();           // Line A
    Derived d;
    d.func();           // Line B
    Base* pb = new Derived();
    pb->func();         // Line C
    Base bb = Derived();  // Line D
    bb.func();          // Line E
    delete pb;
}
What prints at A, B, C, E?


1) Line A — Base

b is type Base (actual object, not pointer)
→ static type = Base
→ dynamic type = Base (same)
→ calls Base::func()
→ Output: Base 

Virtual dispatch matters when:

static type (pointer/reference type) ≠ dynamic type (actual object)
Example: Base* pb = new Derived()
         static type = Base*
         dynamic type = Derived
         → virtual dispatch needed


2) Line B — Derived
3) Line C — Derived

Lines D & E — Object Slicing
Base bb = Derived();  // Line D — object slicing!
bb.func();            // Line E

Derived() creates temporary Derived object
Assigned to bb which is type Base (not pointer)
→ Derived object is SLICED
→ only Base part copied into bb
→ bb becomes pure Base object
→ bb.func() calls Base::func()
Memory:
Derived temp: [vptr→Derived][Derived data]
              ↓ copy only Base part
Base bb:      [vptr→Base][Base data]
              Derived parts LOST
Output at Line E: Base



Object Slicing Summary
Base bb = Derived();
→ temporary Derived created
→ copied into Base bb by value
→ Derived's extra parts SLICED OFF
→ vptr reset to Base vtable
→ bb becomes pure Base object
ALWAYS use pointers/references for polymorphism:
Base* pb = new Derived();  ✅ no slicing
Base& rb = d;              ✅ no slicing
Base bb = d;               ❌ slicing!
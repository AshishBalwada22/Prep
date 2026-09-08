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
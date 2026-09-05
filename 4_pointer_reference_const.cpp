Category: const correctness

int x = 10;
int y = 20;
int*       p1 = &x;   // A
const int* p2 = &x;   // B
int* const p3 = &x;   // C
const int* const p4 = &x;  // D

                Can change pointer?    Can change value?
                  (p = &y)              (*p = 99)
──────────────────────────────────────────────────────────
A: int*              ✅ yes                ✅ yes
B: const int*        ✅ yes                ❌ no
C: int* const        ❌ no                 ✅ yes
D: const int* const  ❌ no                 ❌ no

East-West Rule
Split at the *
Everything LEFT  of * → describes the VALUE being pointed to
Everything RIGHT of * → describes the POINTER itself

const int  *  p2      →   value is const  |  pointer is not
   int  *  const p3   →   value is not    |  pointer is const
const int  *  const p4 →  value is const  |  pointer is const


Also
int x = 10;
int y = 20;
const int* p1 = &x;

// here value is constant, not the pointer
p1 =&y;  // ✅ allowed, pointer can change
*p1 = 15; // ❌ not allowed, value cannot change
x = 15;  // ✅ allowed, value can change directly


Another question - 
int x = 10;
const int* p = &x;    // value is constant, pointer is not
int* q = p;           // you can't assign non const pointer to const pointer
                      // first thing you dropped the const behaviour
                      // what will happen?
                      // q point to the same memory where p points to
                      // and we can change the value x through q pointer, which remove the const behaviour

*p = 20;              // value can't be change through pointer, it is wrong
p = nullptr;          // we can reassign the pointer as it can change

---------------------------------------------------------------------------------------------

Pointer vs Reference


#include <iostream>
using namespace std;
void incrementPointer(int* p) {
    p++;           // Line A
    (*p) = 99;     // Line B
}
void incrementReference(int& r) {
    r++;           // Line C
}
int main() {
    int arr[] = {10, 20, 30};
    
    incrementPointer(&arr[0]);
    cout << arr[0] << " " << arr[1] << "\n";  // 10 99
    int x = 5;
    incrementReference(x);
    cout << x << "\n";                         // 6
    return 0;
}


void incrementPointer(int* p) {
    p++;      // ← p is a LOCAL COPY of the pointer
              //   incrementing p moves it to arr[1]
              //   but this does NOT affect the original pointer
    (*p) = 99; // ← writes 99 to arr[1], not arr[0]
}


// Pointer passed by VALUE
void incrementPointer(int* p) {
    p++;    // modifies LOCAL copy of pointer
            // caller's pointer unchanged
            // but (*p) still reaches original memory
}
// Reference — alias to original
void incrementReference(int& r) {
    r++;    // directly modifies caller's variable
            // no copy involved
}




Another question -
// Current — caller's pointer unchanged:
void incrementPointer(int* p) {
    p++;
}
// How to make caller's pointer advance too?
void incrementPointer(???) {
    ???
}
int* ptr = &arr[0];
incrementPointer(ptr);
// after call: ptr should point to arr[1]



// Option 1: reference to pointer
void incrementPointer(int*& p) {
    p++;              // directly modifies caller's pointer
}
incrementPointer(ptr);   // pass pointer directly — & not needed
// Option 2: pointer to pointer  
void incrementPointer(int** p) {
    (*p)++;           // dereference to get pointer, then increment it
}
incrementPointer(&ptr);  // pass ADDRESS of pointer



Important note - 
To modify T from a function:
─────────────────────────────────────────
T        → pass T&   or T*
int      → pass int& or int*
int*     → pass int*& or int**
int**    → pass int**& or int***
Rule: add one level of indirection
      to modify the thing itself

---------------------------------------------------------------------------------------------

Category: Undefined Behavior 

#include <iostream>
using namespace std;
int* getNumber() {
    int x = 42;
    return &x;
}
int main() {
    int* p = getNumber();
    cout << *p << "\n";    // Line A
    
    cout << *p << "\n";    // Line B
    return 0;
}
Three questions:

Does it compile?
What happens at Line A?
What happens at Line B?


// x lives on stack at some address, say 0xFF10
// return that address
// stack frame GONE but memory not wiped

Time →
────────────────────────────────────────────────────────────
getNumber()    │ x=42 at 0xFF10
               │ return 0xFF10
               │ frame released — 42 still at 0xFF10
────────────────────────────────────────────────────────────
Line A         │ *p reads 0xFF10 → 42 (still there, lucky)
────────────────────────────────────────────────────────────
cout runs      │ internally uses stack
               │ overwrites 0xFF10 with its own data
────────────────────────────────────────────────────────────
Line B         │ *p reads 0xFF10 → garbage (overwritten)
────────────────────────────────────────────────────────────

Fix 1 - Return By Value

// Instead of returning ADDRESS of local variable
// return the VALUE itself — gets COPIED to caller
int getNumber() {    // return type is int, not int*
    int x = 42;
    return x;        // 42 is COPIED into caller's stack
}                    // x dies here, but copy already made

int main() {
    int val = getNumber();  // val = 42, safe copy
    cout << val;            // always 42, no UB
}

Stack during return:
getNumber frame: x = 42
                 42 gets COPIED into return register
getNumber frame: GONE
main frame:      val receives 42 from register
                 safe — no dangling pointer

Fix 2 - Heap Allocation

// Heap memory lives FOREVER until you delete it
// Not tied to any function's lifetime
int* getNumber() {
    int* x = new int(42);  // allocated on HEAP, not stack
    return x;              // return heap address — safe!
}
int main() {
    int* p = getNumber();
    cout << *p;            // safe — heap memory still alive
    delete p;              // YOU must clean up
}

Memory layout:
Stack:  getNumber frame → GONE after return
Heap:   [42] at 0xAB00  → STILL ALIVE after return
                           lives until delete called
p = 0xAB00 → points to heap → always valid

Fix 3 - Smart Pointer

#include <memory>
std::unique_ptr<int> getNumber() {
    return std::make_unique<int>(42);
    // heap allocated + RAII wrapped
    // automatically deleted when unique_ptr goes out of scope
}
int main() {
    auto p = getNumber();
    cout << *p;            // safe
}                          // p goes out of scope → auto deleted
                           // no manual delete needed
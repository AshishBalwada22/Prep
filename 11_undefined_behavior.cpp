Undefined Behavior & Traps


Undefined Behavior (UB) → program can do anything, no guarantees
Dangling pointer        → pointer to dead memory
Use-after-free          → access after deletion
Double delete           → delete same pointer twice
Buffer overflow         → write past array bounds


Category: Use-After-Free 

#include <iostream>
using namespace std;
int* createArray() {
    int arr[10] = {1, 2, 3, 4, 5};
    return arr;      // Line A
}
int main() {
    int* p = createArray();
    cout << p[0] << "\n";   // Line B
    cout << p[1] << "\n";   // Line C
}
Three questions:

Does it compile?
What happens at Line B and C?
What is the bug, and how do you fix it?



int arr[10] = {1, 2, 3, 4, 5};
return arr;      // arr is stack allocated
                 // dies when function returns
                 // pointer to dead memory


It DOES Compile

return arr;      
return &arr[0];  // ✅ same thing

int* p = createArray();
*p          // ✅ dereference first element
*(p+1)      // ✅ dereference second element
p[0]        // ✅ equivalent to *p
p[1]        // ✅ equivalent to *(p+1)


int* createArray() {
    int arr[10] = {1, 2, 3, 4, 5};
    return arr;      // ❌ DANGLING POINTER
}
int main() {
    int* p = createArray();
    cout << p[0] << "\n";   // ❌ undefined behavior
    cout << p[1] << "\n";   // ❌ undefined behavior
}

What happens at runtime:

Might print garbage
Might crash
Might appear to work (memory not yet overwritten)
Unpredictable



Three Fixes - 

Fix 1 — Return by Value (Simplest)

vector<int> createArray() {
    vector<int> arr = {1, 2, 3, 4, 5};
    return arr;      // ✅ copied to caller
}
int main() {
    vector<int> v = createArray();
    cout << v[0] << "\n";   // ✅ safe
}


Fix 2 — Heap Allocation

int* createArray() {
    int* arr = new int[10];
    arr[0] = 1;
    arr[1] = 2;
    return arr;      // ✅ heap memory lives beyond function
}
int main() {
    int* p = createArray();
    cout << p[0] << "\n";   // ✅ safe
    delete[] p;             // ❌ caller must remember to delete
}


Fix 3 — Modern C++ (Best)

unique_ptr<int[]> createArray() {
    auto arr = make_unique<int[]>(10);
    arr[0] = 1;
    arr[1] = 2;
    return arr;      // ✅ RAII, auto deleted
}
int main() {
    auto p = createArray();
    cout << p[0] << "\n";   // ✅ safe
}                           // ✅ auto deleted



What Interviewer Expects
This is a dangling pointer — returning address of a local variable. 
The local lives on the stack and its lifetime ends when the function returns. Dereferencing it is undefined behavior. 
Fix: return by value (vector), heap allocate (new/delete), or use smart pointers (unique_ptr). 
Modern C++ prefers returning by value or smart pointers.


------------------------------------------------------------------------------------------------------------------------------------

Category: Double Delete

#include <iostream>
using namespace std;
int main() {
    int* p = new int(42);
    
    cout << *p << "\n";     // Line A
    
    delete p;               // Line B
    
    cout << *p << "\n";     // Line C
    
    delete p;               // Line D
}
Three questions:

Does it compile?
What happens at each line?
What is the exact bug?


Line A: cout << *p << "\n";     // 42 ✅
Line B: delete p;               // pointer still holds address, but memory freed
Line C: cout << *p << "\n";     // undefined behavior ✅
Line D: delete p;               // double delete ✅ CRASH/UB


on Line C - 

After delete p:
p still holds the address (pointer value unchanged)
But memory at that address is freed (no longer owned)
Reading *p is undefined behavior
Possible outcomes:
1. Memory not overwritten yet → prints 42 (looks correct!)
2. Memory overwritten → prints garbage
3. Program crashes (segmentation fault)
4. Anything else — undefined behavior


on Line D -

delete p;               // Line B — frees memory
delete p;               // Line D — frees SAME memory again
                        // ❌ UNDEFINED BEHAVIOR
                        // ❌ heap corruption
                        // ❌ program crashes



Complete Execution Flow
new int(42)
↓
p = 0x1000, memory contains 42
↓
Line A: *p → 42 ✅
↓
delete p
↓
p = 0x1000 (pointer unchanged), memory freed (no longer owned)
↓
Line C: *p → undefined behavior (garbage or 42)
↓
delete p
↓
heap corruption → CRASH ❌



Fixes
Fix 1 — Set to nullptr after delete
int* p = new int(42);
delete p;
p = nullptr;            // ✅ mark as invalid
cout << *p << "\n";     // ❌ still UB, but easier to catch
delete p;               // ✅ safe (delete nullptr is no-op)
Fix 2 — Use Smart Pointers (Best)
unique_ptr<int> p(new int(42));
cout << *p << "\n";     // 42 ✅
// p automatically deleted when goes out of scope
// cannot double delete
// cannot use-after-free
Fix 3 — Modern Construction
auto p = make_unique<int>(42);
cout << *p << "\n";     // 42 ✅
// automatic cleanup, no manual delete needed

What Interviewer Expects
Double delete is undefined behavior — deleting the same pointer twice causes heap corruption and usually crashes.
The pointer value doesn't change after delete, so it still holds the old address, but that memory is no longer owned. 
Reading or deleting it again is undefined behavior.
Fix: set pointer to nullptr after delete, or use smart pointers which prevent double delete automatically.

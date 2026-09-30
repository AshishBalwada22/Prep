#include <iostream>
using namespace std;

class Buffer {
    int* data;
public:
    Buffer(int size) {
        data = new int[size];
        cout << "Constructor\n";
    }

    ~Buffer() {
        cout << "Destructor\n";
        delete[] data;
    }

    // Move Constructor
    Buffer(Buffer&& other) noexcept {
        data = other.data;
        other.data = nullptr;
        cout << "Move Constructor\n";
    }

    // Delete copy constructor
    Buffer(const Buffer&) = delete;
};

Buffer createBuffer() {
    Buffer b(100);
    return b;      // Line A — what happens here?
}

int main() {
    Buffer result = createBuffer();  // Line B
    cout << "Done\n";
}


Three questions:

What prints and in what order?
Does the code have memory leaks?
What is Return Value Optimization (RVO) and does it apply here?



WITHOUT RVO (old way):
Buffer b(100);           // construct in function
Buffer temp = move(b);   // move to temp (in caller's frame)
Buffer result = move(temp);  // move to result
// 3 objects, 2 moves, 2 destructors

WITH RVO (C++17+ mandatory):
// Construct b DIRECTLY in result's memory
// No temp, no move needed
// 1 object, 0 moves, 1 destructor


Actual Output (With RVO)
Constructor      ← b(100) constructed directly as result
Done
Destructor       ← result destroyed at end



Why RVO Works Here
Buffer createBuffer() {
    Buffer b(100);
    return b;      // ← GUARANTEED RVO
}
Compiler recognizes:

Returning a local variable by value
Same variable in all return paths (if multiple)
Compiler MUST elide the copy/move
Direct construction in caller's memory

If RVO Didn't Apply (Hypothetically)
Buffer createBuffer() {
    Buffer b(100);
    return b;      // no RVO somehow
}
Buffer result = createBuffer();
Then:

Constructor      ← b(100)
Move Constructor ← return b (b is lvalue, but compiler forces move)
Destructor       ← b destroyed in function
Destructor       ← result destroyed at end



RVO Rules — C++17+
Guaranteed RVO (Named Return Value Optimization):
→ Return local variable by value
→ Compiler MUST elide the copy/move
→ No temporary created
→ Direct construction in caller's memory
Example:
Buffer createBuffer() {
    Buffer b(100);
    return b;    // ← guaranteed RVO, no move
}


What Interviewer Expects
"Modern C++ compilers (C++17+) guarantee Return Value Optimization (RVO) for returning local variables by value. 
The compiler directly constructs the object in the caller's memory, eliminating temporary objects and move operations.
This is why returning by value is now preferred over returning by reference or pointer. 
RVO is mandatory by the standard, not just an optimization."



------------------------------------------------------------------------------------------

#include <iostream>
using namespace std;
class Obj {
    int id;
public:
    Obj(int id) : id(id) { cout << "Obj(" << id << ")\n"; }
    ~Obj() { cout << "~Obj(" << id << ")\n"; }
    
    Obj(Obj&& other) noexcept : id(other.id) {
        cout << "Move(" << id << ")\n";
    }
    
    Obj(const Obj&) = delete;
};
int main() {
    Obj o1(1);
    Obj o2 = move(o1);      // Line A
    
    cout << "---\n";
    
    Obj o3(3);
    Obj o4 = o3;            // Line B
}


Line A: move(o1) calls Move Constructor → prints "Move(1)" ✅
Line B: o3 is lvalue, copy constructor deleted → doesn't compile ✅


Output(If Line B is commented out):

Obj(1)           ← o1 constructed
Move(1)          ← o2 move constructed
---
Obj(3)           ← o3 constructed
~Obj(3)          ← o3 destroyed (reverse order)
~Obj(1)          ← o2 destroyed (id is still 1)
~Obj(1)          ← o1 destroyed (id is still 1)


1. move(o1) converts o1 to rvalue
2. Move constructor called: Obj(Obj&& other)
3. o2 is constructed with other.id
4. o1 STILL EXISTS — move doesn't destroy it
5. Both o1 and o2 are alive
6. When main() ends, BOTH destructors called



move() ≠ delete
move() means:
→ steal resources from rvalue
→ original still exists
→ original is in "moved-from" state
→ original will be destroyed later
Example:
unique_ptr<int> p1(new int(42));
unique_ptr<int> p2 = move(p1);
p1 still exists, but p1.get() == nullptr
p2 owns the memory
Both destroyed at end of scope
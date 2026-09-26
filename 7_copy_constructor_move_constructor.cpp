Move Constructor vs Copy — Performance

#include <iostream>
#include <vector>
using namespace std;

class Buffer {
    int* data;
    int size;
public:
    // Constructor
    Buffer(int sz) : size(sz) {
        data = new int[sz];
        cout << "Constructor\n";
    }

    // Destructor
    ~Buffer() {
        delete[] data;
    }

    // Copy Constructor
    Buffer(const Buffer& other) : size(other.size) {
        data = new int[other.size];
        memcpy(data, other.data, other.size * sizeof(int));
        cout << "Copy Constructor\n";
    }

    // Move Constructor
    Buffer(Buffer&& other) noexcept : size(other.size) {
        data = other.data;           // steal pointer
        other.data = nullptr;        // leave other empty
        other.size = 0;
        cout << "Move Constructor\n";
    }
};

int main() {
    Buffer b1(10);                   // Line A

    Buffer b2 = b1;                  // Line B

    Buffer b3 = Buffer(20);          // Line C

    Buffer b4 = move(b1);            // Line D
}


A: Constructor          (b1(10) calls constructor)
B: Copy Constructor     (b2 = b1, b1 is lvalue)
D: Move Constructor     (move(b1) converts to rvalue)


Buffer b3 = Buffer(20);  // Line C

Buffer(20)  → creates temporary Buffer object
             → calls Constructor
             → prints "Constructor"
b3 = Buffer(20)  → assigns temporary to b3
                 → Buffer(20) is rvalue (temporary)
                 → calls Move Constructor
                 → prints "Move Constructor"


 Why Move, Not Copy?
Buffer(20) is rvalue (temporary)
→ rvalue binds to rvalue reference (Buffer&&)
→ Move Constructor called
→ steals pointer from temporary
→ efficient!
If it were lvalue:
Buffer b_temp = Buffer(20);
Buffer b3 = b_temp;  // would call Copy Constructor
→ allocates new memory
→ copies data
→ slower


Constructor          ← Line A: b1(10)
Copy Constructor     ← Line B: b2 = b1 (lvalue)
Constructor          ← Line C: Buffer(20) creates temp
Move Constructor     ← Line C: b3 = temp (rvalue)
Constructor          ← Line D: b1 created earlier
Move Constructor     ← Line D: move(b1) forces rvalue


Copy vs Move — Performance Comparison
Copy Constructor:
→ allocate new memory
→ memcpy all data
→ O(n) time, O(n) space
→ SLOW for large buffers
Move Constructor:
→ steal pointer
→ no allocation
→ O(1) time, O(1) space
→ FAST regardless of size


------------------------------------------------------------------------------------------

Smart Pointers — Ownership

#include <memory>
using namespace std;

class Resource {
public:
    ~Resource() { cout << "~Resource\n"; }
};

int main() {
    // A
    unique_ptr<Resource> u1(new Resource());
    unique_ptr<Resource> u2 = u1;           // Line A — does this compile?

    // B
    shared_ptr<Resource> s1(new Resource());
    shared_ptr<Resource> s2 = s1;           // Line B — does this compile?

    // C
    unique_ptr<Resource> u3 = move(u1);     // Line C — does this compile?

    // D
    shared_ptr<Resource> s3 = s1;           // Line D — does this compile?
    shared_ptr<Resource> s4 = s1;           // Line E — does this compile?
}
// What prints and when?

For each line:

Does it compile?
What happens to reference count / ownership?
At end of main, what prints and in what order?


A: unique_ptr<Resource> u2 = u1;    ❌ does NOT compile
   Reason: unique_ptr cannot be copied (sole ownership)
B: shared_ptr<Resource> s2 = s1;    ✅ compiles
   Reason: shared_ptr can be copied (shared ownership)
C: unique_ptr<Resource> u3 = move(u1);  ✅ compiles
   Reason: unique_ptr can be moved (ownership transfer)
D: shared_ptr<Resource> s3 = s1;    ✅ compiles
   Reason: shared_ptr can be copied
E: shared_ptr<Resource> s4 = s1;    ✅ compiles
   Reason: shared_ptr can be copied



int main() {
    unique_ptr<Resource> u1(new Resource());
    // Resource created
    // u1 owns it
    shared_ptr<Resource> s1(new Resource());
    // 2nd Resource created
    // s1 owns it, ref_count = 1
    shared_ptr<Resource> s2 = s1;
    // s2 shares ownership
    // ref_count = 2
    unique_ptr<Resource> u3 = move(u1);
    // u3 steals from u1
    // u1 becomes nullptr (owns nothing)
    // u3 owns 1st Resource
    shared_ptr<Resource> s3 = s1;
    // s3 shares ownership
    // ref_count = 3
    shared_ptr<Resource> s4 = s1;
    // s4 shares ownership
    // ref_count = 4
}
// end of scope:


Destruction Order

u3 goes out of scope
→ u3 owns 1st Resource (sole owner)
→ deletes it
→ ~Resource prints (1st Resource)

s4 goes out of scope
→ ref_count: 4 → 3

s3 goes out of scope
→ ref_count: 3 → 2

s2 goes out of scope
→ ref_count: 2 → 1

s1 goes out of scope
→ ref_count: 1 → 0
→ deletes 2nd Resource
→ ~Resource prints (2nd Resource)

u1 goes out of scope
→ u1 owns nothing (was moved)
→ nothing happens


~Resource   ← u3 deleted (1st Resource)
~Resource   ← s1 deleted (2nd Resource, ref_count reached 0)


Ownership Model Comparison
unique_ptr:
→ sole owner
→ cannot copy
→ can move (ownership transfer)
→ when unique_ptr dies → delete happens
→ only ONE unique_ptr owns at a time
→ zero overhead (no ref count)
shared_ptr:
→ shared owner
→ can copy
→ reference count tracks ownership
→ when last shared_ptr dies → delete happens
→ MANY shared_ptrs can own simultaneously
→ small overhead (ref count)
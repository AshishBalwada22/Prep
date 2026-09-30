
Category: Iterator Invalidation — Hidden Bugs

#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    auto it = v.begin() + 1;  // points to 20
    cout << *it << "\n";      // Line A - 20

    v.push_back(60);          // Line B

    cout << *it << "\n";      // Line C — is it valid?

    v.erase(it);              // Line D

    cout << v.size() << "\n"; // Line E
}


Vector Capacity vs Size — The Hidden Trap

vector<int> v = {10, 20, 30, 40, 50};
// size = 5
// capacity = ? (at least 5, often more)
v.push_back(60);
// size = 6
// capacity = still same (if it was > 5)
// OR reallocated (if capacity was exactly 5)

Scenario 1 — Capacity > Size (No Reallocation)
Before: [10, 20, 30, 40, 50, ???, ???]
         size=5, capacity=7
After push_back:
        [10, 20, 30, 40, 50, 60, ???]
         size=6, capacity=7
it still points to index 1 → 20 ✅ valid
Scenario 2 — Capacity == Size (Reallocation)
Before: [10, 20, 30, 40, 50]
         size=5, capacity=5
After push_back:
        NEW MEMORY allocated (usually 2x capacity)
        [10, 20, 30, 40, 50, 60, ???, ???, ???, ???]
         size=6, capacity=10
it points to OLD memory → DANGLING ❌ undefined behavior


The Real Answer for Line C
Line C: *it might be valid or undefined behavior
        Depends on whether vector reallocated
        In practice: unpredictable
        
Best answer: "Undefined behavior — iterator invalidated by push_back"


v.erase(it);    // ❌ undefined behavior
Why? After Line C, it might be:

Valid (if no reallocation) → erase works
Dangling (if reallocation happened) → undefined behavior


Iterator Invalidation Rules — Memorize These
vector:
push_back()     → invalidates all iterators IF reallocation
insert()        → invalidates all iterators after insertion point
erase()         → invalidates iterators at/after erased element




The Safe Pattern
vector<int> v = {10, 20, 30, 40, 50};
auto it = v.begin() + 1;
it = v.erase(it);    // erase returns iterator to next element
                     // safe to use returned iterator
cout << *it << "\n"; // safe — points to 30 now
Or avoid iterator invalidation altogether:

vector<int> v = {10, 20, 30, 40, 50};
v.push_back(60);     // no iterator used
v.erase(v.begin() + 1);  // create fresh iterator


----------------------------------------------------------------------------------------

Summary — Container Choice
Access pattern              → Container
─────────────────────────────────────────
Both ends frequently        → deque
Random access + back ops    → vector
Sorted iteration            → map
Fast key lookup             → unordered_map
Middle operations           → list

----------------------------------------------------------------------------------------
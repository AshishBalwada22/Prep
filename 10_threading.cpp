Threading

mutex       → lock/unlock, one thread at a time
lock_guard  → RAII wrapper, auto unlock
race condition → multiple threads modify same data unsynchronized
deadlock    → threads waiting for each other in circle
atomic      → thread-safe primitive types


Category: Race Condition

#include <thread>
#include <iostream>
using namespace std;
int counter = 0;
void increment() {
    for (int i = 0; i < 1000000; i++) {
        counter++;          // Line A
    }
}
int main() {
    thread t1(increment);
    thread t2(increment);
    t1.join();
    t2.join();
    cout << counter << "\n";  // Line B — what prints?
}
Three questions:

Is there a race condition at Line A?
What should counter be at Line B?
What might it actually print? Why?


Race condition?        YES
Expected counter:      2000000
Actual output:         Less than 2000000 (unpredictable)
Root cause:            counter++ is not atomic


Three Fixes - 

Fix 1 — mutex + lock_guard

#include <mutex>
int counter = 0;
mutex m;
void increment() {
    for (int i = 0; i < 1000000; i++) {
        lock_guard<mutex> lock(m);  // acquire lock
        counter++;                  // safe — only one thread
    }                               // lock released here (RAII)
}

Downside: Locking overhead, slow.


Fix 2 — atomic

#include <atomic>
atomic<int> counter(0);  // thread-safe counter
void increment() {
    for (int i = 0; i < 1000000; i++) {
        counter++;       // atomic operation, no lock needed
    }
}
Upside: Fast, lock-free on most platforms.


-----------------------------------------------------------------------------------------------------------------------------------

Category: Deadlock 

#include <thread>
#include <mutex>
using namespace std;
mutex m1, m2;
void threadA() {
    lock_guard<mutex> lock1(m1);      // acquire m1
    this_thread::sleep_for(chrono::milliseconds(10));
    lock_guard<mutex> lock2(m2);      // wait for m2
}
void threadB() {
    lock_guard<mutex> lock2(m2);      // acquire m2
    this_thread::sleep_for(chrono::milliseconds(10));
    lock_guard<mutex> lock1(m1);      // wait for m1
}
int main() {
    thread t1(threadA);
    thread t2(threadB);
    t1.join();
    t2.join();
}
Three questions:

Is there a deadlock?
What is the sequence of events that causes it?
How do you fix it?


Deadlock?              YES
Sequence of events:    Circular wait (A holds m1, wants m2; B holds m2, wants m1)
Fix:                   Acquire locks in same order
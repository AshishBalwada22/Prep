RAII — Design + Bug Finding

#include <iostream>
#include <stdexcept>
void processFile() {
    FILE* f = fopen("data.txt", "r");
    
    // ... do some work ...
    if (someErrorCondition) {
        throw std::runtime_error("something went wrong");
    }
    // ... do more work ...
    fclose(f);
}


Three questions:

What is the bug in this code?
What is RAII and how does it fix this?
Write a simple RAII wrapper for FILE* that fixes the bug


what is the issue coming - 
void processFile() {
    FILE* f = fopen("data.txt", "r");   // resource acquired
    // Path 1: no error
    // ...work...
    fclose(f);                           // resource released ✅
    // Path 2: error occurs
    // ...work...
    throw std::runtime_error("...");     // ← what happens to fclose?
    // ...
    fclose(f);                           // does this line execute?
}


simple RAII wrapper class for FILE*

class FileGuard {
    FILE* f;                    // raw resource we are wrapping
public:
    FileGuard(const char* filename, const char* mode) {
        f = fopen(filename, mode);
        if (!f) {
            throw std::runtime_error("Failed to open file");
        }
    }
    ~FileGuard() {
        if (f) {
            fclose(f);        
        }
    }
    // Disable copy — two FileGuards closing same FILE* = double close
    FileGuard(const FileGuard&)            = delete;
    FileGuard& operator=(const FileGuard&) = delete;
};


Why `= delete` on Copy?
FileGuard a("data.txt", "r");
FileGuard b = a;          // ❌ now two objects own same FILE*
// end of scope:
// ~FileGuard() for b → fclose(f) ✅
// ~FileGuard() for a → fclose(f) ❌ DOUBLE CLOSE — undefined behavior
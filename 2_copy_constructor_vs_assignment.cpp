Copy Constructor vs Assignment — Bug Finding

class Buffer {
    int* data;
    int  size;
public:
    Buffer(int sz) : size(sz) {
        data = new int[sz];
    }
    ~Buffer() {
        delete[] data;
    }
};
int main() {
    Buffer b1(10);
    Buffer b2 = b1;   // Line A
    Buffer b3(5);
    b3 = b1;          // Line B
    return 0;
}
Three specific questions:

What happens at Line A — which function is called?
What happens at Line B — which function is called?
What is the exact bug, and when does the crash happen?

if the object is being initialized, then copy constructor invoked
if the object already exist, then copy assignment operator called

// Compiler generated copy constructor does this:
Buffer(const Buffer& other) {
    data = other.data;   // ← shallow copy, SAME pointer
    size = other.size;
}

b2 = b1;// b2.data and b1.data point to the same memory (shallow copy)

destructor invoked in reverse order - 

delete[] b3.data -> fine
delete[] b2.data -> fine, free up the memory
delete[] b1.data -> crash, double free of the same memory

Rule of 3 says, if you define the the destructor, then you need to define copy constructor and copy assignment operator as well.

Rule 0f 5 says, if you define the destructor, copy constructor, and copy assignment operator, then you also need to define move constructor and move assignment operator as well.

Rule of 0 says, use smart pointer instead, RAII



correct implementation - 

class Buffer {
    int* data;
    int  size;
public:
    Buffer(int sz) : size(sz) {
        data = new int[sz];
    }
    ~Buffer() {
        delete[] data;
    }
    // copy constructor - deep copy
    Buffer(const Buffer& other) : size(other.size){
        // allocate new memory for the copy
        data = new int[other.size];
        memcpy(data,other.data, size*sizeof(int)); // number of element * size of int = total byte
    }
    // copy assignment operator - deep copy
    Buffer& operator=(const Buffer& other) {
        // self assignment check
        if(*this == &other) return *this;
        // free up the space of existing pointer
        delete[] data;
        size = other.size;
        data = new int[other.size];
        memcpy(data,other.data,other.size*sizeof(int));
    }
    return *this;
};
int main() {
    Buffer b1(10);
    Buffer b2 = b1;   // Line A
    Buffer b3(5);
    b3 = b1;          // Line B
    return 0;
}
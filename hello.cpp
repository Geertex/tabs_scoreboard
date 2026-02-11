#include <iostream>

int main() {
	std::cout << "hello";

    return 0;
}

// clang-cl hello.cpp /EHsc /link /SUBSYSTEM:CONSOLE
// clang-cl is the compiler program
// hello.cpp is our code
// /EHsc is a shorthand for how the compiler handles C++ exceptions.
// /link /SUBSYSTEM:CONSOLE means the entry point is linked to the console
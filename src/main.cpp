#include <iostream>
#include "bpp_shared/numeric_structs.h"

int main(int argc, char** argv) {
    Int3 int3{2,156,16};
    std::cout << int3 << std::endl;
    std::cout << int3.x << std::endl;
    std::cout << int3.data[2] << std::endl;

    return 0;
}
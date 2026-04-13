#include <iostream>
#include "bpp_shared/numeric_structs.h"

int main(int argc, char** argv) {
    Int3 int3{2,156,16};
    std::cout << int3 << std::endl;
    std::cout << int3.x << std::endl;
    std::cout << int3.data[2] << std::endl;
    int3 = int3*5;
    std::cout << int3 << std::endl;

    Vec3 vec3{2.5, 1235.5134, 5245.2};
    std::cout << vec3 << std::endl;
    std::cout << vec3.x << std::endl;
    std::cout << vec3.data[2] << std::endl;
    vec3 = vec3*5;
    std::cout << vec3 << std::endl;
    vec3 = vec3*2.1;
    std::cout << vec3 << std::endl;

    
    Byte2 b2;
    b2 = Byte2{5,16};
    std::cout << b2 << std::endl;

    return 0;
}
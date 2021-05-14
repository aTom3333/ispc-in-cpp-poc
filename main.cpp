#include <iostream>
#include "include/varying.hpp"
#include "include/control_flow.hpp"


using iic::varying;

varying<float> max(varying<float> a, const varying<float>& b)
{
    iic_if(b > a)
        a = b;
    return a;
}

int main()
{
    float arr[] = {
        1, 2, 3, 4, 5, 6, 7, 8
    };
    float* p = arr;
    
    std::cout << "Hello, World!" << std::endl;
    iic::varying<float> a = 4;
    iic::varying<float> b = 7;
    
    std::cout << a << std::endl;
    std::cout << b << std::endl;
    
    iic::_current_mask._values[1] = false;
    
    a = b;
    
    std::cout << a << std::endl;
    
    iic::_current_mask._values[2] = false;
    
    auto c = a * 2 - 3;

    iic::_current_mask._values[1] = true;
    iic::_current_mask._values[2] = true;
    
    std::cout << c << std::endl;
    std::cout << max(a, c) << std::endl;
    
    auto d = a;
    
    std::cout << d << std::endl;
    
    iic::varying<int> offset;
    offset._values[1] = 1;
    offset._values[2] = 2;
    offset._values[3] = 3;
    
    iic::varying<float*> truc = p + offset;
    *truc = d;
    
    
    return 0;
}
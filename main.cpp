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

void thingy(int start, int end)
{
    if(start < 1)
        start = 1;
    if(end < start)
        end = start;
    
    iic_foreach(auto number : iic::range(start, end))
    {
        iic::varying<int> iteration = 0;
        iic_while(number != 1)
        {
            iic_if(number % 2 == 0)
                number /= 2;
            else
                number = number * 3 + 1;
            ++iteration;
        }
        
        std::cout << iteration << std::endl;
    }
}

int main()
{
    float arr[] = {
        1, 2, 3, 4, 5, 6, 7, 8
    };
    const float* p = arr;
    
    std::cout << "Hello, World!" << std::endl;
    iic::varying<float> a = 4;
    iic::varying<float> b = 7;
    
    std::cout << a << std::endl;
    std::cout << b << std::endl;
    
    iic::_current_mask._values[1] = false;
    
    a = b;
    
    std::cout << a << std::endl;
    
    iic::_current_mask._values[2] = false;
    
    iic_foreach_active(lane)
        std::cout << lane << " ";
    std::cout << std::endl;
    
    auto c = a * 2 - 3;

    iic_unmasked
    {
        std::cout << c << std::endl;
        std::cout << max(a, c) << std::endl;
    }
    
    auto d = a;
    
    std::cout << d << std::endl;
    
    iic::varying<int> offset;
    offset._values[1] = 1;
    offset._values[2] = 2;
    offset._values[3] = 3;
    
    iic::varying<const float*> truc = p + offset;
    //*truc = d;
    
    iic_foreach(auto i : iic::range(0, 7))
    {
        std::cout << i << std::endl;
        varying<float> lane = *(p + i);
        std::cout << lane << std::endl;
    }
    
    
    thingy(0, 30);
    
    
    return 0;
}

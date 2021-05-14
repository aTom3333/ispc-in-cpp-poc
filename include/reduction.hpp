#ifndef REDUCTION_HPP
#define REDUCTION_HPP

#include "varying.hpp"

namespace iic
{
    bool all(const varying<bool>& input)
    {
        bool val = true;
        for(const auto& lane : input._values)
            val = val && lane;
        return val;
    }
    
    bool any(const varying<bool>& input)
    {
        bool val = false;
        for(const auto& lane : input._values)
            val = val || lane;
        return val;
    }
    
    bool none(const varying<bool>& input)
    {
        return !any(input);
    }
    
    // TODO Do more (maybe)
}

#endif // REDUCTION_HPP
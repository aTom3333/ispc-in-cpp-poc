/*
 * zlib License
 *
 * (C) 2021 Thomas FERRAND
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

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
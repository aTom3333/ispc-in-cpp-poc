#ifndef CONTROL_FLOW_HPP
#define CONTROL_FLOW_HPP

#include "varying.hpp"

namespace iic
{
    namespace detail
    {
        template<bool is_varying>
        struct if_state;
        
        template<>
        struct if_state<false>
        {
            static constexpr bool is_varying = false;
            
            bool condition;
        };
        
        template<>
        struct if_state<true>
        {
            static constexpr bool is_varying = true;
            
            if_state(const mask_t& cond):
                condition(Private{}, cond._values),
                old_mask(Private{}, _current_mask._values)
            {
                _current_mask._values = compute_mask();
            }
            
            std::array<bool, LANE_SIZE> compute_mask()
            {
                auto helper = [&]<std::size_t... I>(std::index_sequence<I...>)
                {
                    return std::array<bool, LANE_SIZE> {
                        (old_mask._values[I] && condition._values[I])...
                    };
                };
                return helper(std::make_index_sequence<LANE_SIZE>{});
            }
            
            void invert()
            {
                auto helper = [&]<std::size_t... I>(std::index_sequence<I...>)
                {
                    ((condition._values[I] = !condition._values[I]), ...);
                };
                helper(std::make_index_sequence<LANE_SIZE>{});
                _current_mask._values = compute_mask();
            }
            
            ~if_state()
            {
                _current_mask._values = old_mask._values;
            }
            
            mask_t condition;
            mask_t old_mask;
        };
        
        inline if_state<false> make_if_state(bool cond)
        {
            return if_state<false>{cond};
        }
        
        inline if_state<true> make_if_state(const mask_t& cond)
        {
            return if_state<true>(cond);
        }
        
        inline bool should_goto_else(const if_state<false>& state)
        {
            return !state.condition;
        }
        
        inline bool should_goto_else(const if_state<true>& state)
        {
            return false;
        }
    }
}


#define CONCAT(a, b) a##b
#define CAT(a, b) CONCAT(a, b)

// To get a explanation of this madness, see https://www.chiark.greenend.org.uk/~sgtatham/mp/
#define iic_if(cond) \
if (0) {             \
    /* after the if and the else */                \
    CAT(finished, __LINE__): ; \
} \
else \
    for (auto state = ::iic::detail::make_if_state(cond) ;;) \
        if(1) {      \
            /* before the if */         \
            if constexpr (!decltype(state)::is_varying) {      \
                if(::iic::detail::should_goto_else(state)) \
                    goto CAT(else_part, __LINE__); \
            }\
            goto CAT(body, __LINE__); \
        } else \
            while(1) \
                if(1) \
                    goto CAT(finished, __LINE__); \
                else CAT(else_part, __LINE__): \
                    if(0) \
                        while (1) \
                            if (1) {  \
                                /* after if body but before else body */ \
                                if constexpr(!decltype(state)::is_varying) \
                                    goto CAT(finished, __LINE__); \
                                else  \
                                    state.invert(); \
                                goto CAT(else_part, __LINE__); \
                            } else    \
                                /* if body */ \
                                CAT(body, __LINE__):
                    /* else body */


#endif // CONTROL_FLOW_HPP
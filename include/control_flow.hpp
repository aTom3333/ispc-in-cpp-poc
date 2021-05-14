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
        
        std::array<bool, LANE_SIZE> internal_and(const std::array<bool, LANE_SIZE>& a, const std::array<bool, LANE_SIZE>& b)
        {
            auto helper = [&]<std::size_t... I>(std::index_sequence<I...>)
            {
                return std::array<bool, LANE_SIZE> {
                    (a[I] && b[I])...
                };
            };
            return helper(std::make_index_sequence<LANE_SIZE>{});
        }
        
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
                return internal_and(old_mask._values, condition._values);
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
        
        struct restore_mask
        {
            mask_t old_mask;

            restore_mask():
                old_mask(Private{}, _current_mask._values)
            {}

            ~restore_mask()
            {
                _current_mask._values = old_mask._values;
            }
        };
        
        struct unmasked_state : restore_mask
        {
            unmasked_state():
                restore_mask()
            {
                _current_mask._values = all_true(std::make_index_sequence<LANE_SIZE>{});
            }
        };
        
        template<bool is_varying>
        struct while_state;
        
        template<>
        struct while_state<false>
        {
            bool iter(bool condition)
            {
                return condition;
            }
        };
        
        template<>
        struct while_state<true> : restore_mask
        {
            bool iter(const mask_t& condition)
            {
                _current_mask._values = internal_and(_current_mask._values, condition._values);
                return std::any_of(_current_mask._values.begin(), _current_mask._values.end(), [](bool b){ return b; });
            }
        };
        
        template<typename T>
        static constexpr bool always_false = false; 
        
        template<typename T>
        auto make_while_state()
        {
            if constexpr(std::is_convertible_v<T, mask_t>)
                return while_state<true>{};
            else if constexpr(std::is_convertible_v<T, bool>)
                return while_state<false>{};
            else
                static_assert(always_false<T>, "Bad type");
        }
    }

    template<typename T>
    struct range
    {
        range(T s, T f): start(s), finish(f) {}

        T start, finish;

        struct iterator
        {
            T current;
            T finish;

            bool operator!=(const iterator& other)
            {
                return current != other.current;
            }

            detail::varying_impl<T> operator*()
            {
                std::array<bool, detail::LANE_SIZE> new_mask{};
                std::array<T, detail::LANE_SIZE> new_values{};
                for(int i = 0; i < detail::LANE_SIZE && current != finish; ++current, ++i)
                {
                    new_mask[i] = true;
                    new_values[i] = current;
                }
                _current_mask._values = new_mask;
                return detail::varying_impl<T>(detail::Private{}, new_values);
            }

            iterator& operator++() {
                return *this;
            }
        };

        iterator begin()
        {
            return iterator{start, finish};
        }

        iterator end()
        {
            return iterator{finish, finish};
        }
    };
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
    for (auto CAT(state, __LINE__) = ::iic::detail::make_if_state(cond) ;;) \
        if(1) {      \
            /* before the if */         \
            if constexpr (!decltype(CAT(state, __LINE__))::is_varying) {      \
                if(::iic::detail::should_goto_else(CAT(state, __LINE__))) \
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
                                if constexpr(!decltype(CAT(state, __LINE__))::is_varying) \
                                    goto CAT(finished, __LINE__); \
                                else  \
                                    CAT(state, __LINE__).invert(); \
                                goto CAT(else_part, __LINE__); \
                            } else    \
                                /* if body */ \
                                CAT(body, __LINE__):
                    /* else body */

#define iic_internal_mask_restore \
if(0)                \
    CAT(finished, __LINE__): ; \
else                 \
    for(::iic::detail::restore_mask CAT(state, __LINE__) ;;) \
        if(1)        \
            goto CAT(body, __LINE__);           \
        else \
            while(1) \
                if(1)\
                    goto CAT(finished, __LINE__); \
                else \
                    CAT(body, __LINE__):
                    
                    
#define iic_unmasked \
if(0)                \
    CAT(finished, __LINE__): ; \
else                 \
    for(::iic::detail::unmasked_state CAT(state, __LINE__) ;;) \
        if(1)        \
            goto CAT(body, __LINE__);           \
        else \
            while(1) \
                if(1)\
                    goto CAT(finished, __LINE__); \
                else \
                    CAT(body, __LINE__):


#define iic_foreach(decl) \
iic_unmasked \
    for(decl)
            
    
#define iic_while(cond) \
if(0)                   \
    CAT(finished, __LINE__): ; \
else                    \
    for(auto CAT(state, __LINE__) = ::iic::detail::make_while_state<decltype(cond)>();;) \
        if(1)           \
            goto CAT(body, __LINE__);                                   \
        else            \
            while(1)    \
                if(1)   \
                    goto CAT(finished, __LINE__);                       \
                else \
                    CAT(body, __LINE__):                                \
                        while(CAT(state, __LINE__).iter(cond))
                
                
#endif // CONTROL_FLOW_HPP
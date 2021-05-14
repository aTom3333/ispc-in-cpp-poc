#ifndef VARYING_HPP
#define VARYING_HPP

#include <concepts>
#include <cstddef>
#include <array>
#include <iosfwd>


namespace iic
{
    namespace detail
    {
        constexpr size_t LANE_SIZE = 4;


        #define FOR_ALL_ASSIGNABLE_OP(MACRO) \
            MACRO(+) \
            MACRO(-) \
            MACRO(*) \
            MACRO(/) \
            MACRO(%) \
            MACRO(&) \
            MACRO(|) \
            MACRO(^) \
            MACRO(<<) \
            MACRO(>>)

        #define FOR_ALL_OP(MACRO) \
            FOR_ALL_ASSIGNABLE_OP(MACRO) \
            MACRO(&&) \
            MACRO(||)                 \
            MACRO(==)                  \
            MACRO(!=)                 \
            MACRO(<)                  \
            MACRO(>)                  \
            MACRO(<=)                 \
            MACRO(>=)
        
        
        struct Private {};
        
        template<typename T>
        struct varying_reference;
        
        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>
        struct varying_impl
        {
            using type = T;

            varying_impl();
            
            varying_impl(const varying_impl& other);
            varying_impl(varying_impl&& other);

            // Implicit construction
            template<typename U>
            requires std::convertible_to<U, T>
            varying_impl(const varying_impl<U>& other);
            template<typename U>
            requires std::convertible_to<U, T>
            varying_impl(const U& other);

            // Explicit construction
            template<typename U>
            requires std::constructible_from<T, U>
                     && (!std::convertible_to<U, T>)
            explicit varying_impl(const varying_impl<U>& other);
            template<typename U>
            requires std::constructible_from<T, U>
                     && (!std::convertible_to<U, T>)
            explicit varying_impl(const U& other);

            // Assignment
            varying_impl& operator=(const varying_impl& other);
            varying_impl& operator=(varying_impl&& other);
            
            template<typename U>
            requires std::convertible_to<U, T>
            varying_impl& operator=(const varying_impl<U>& other);
            template<typename U>
            requires std::convertible_to<U, T>
            varying_impl& operator=(const U& other);

            // TODO Add concept to all this 
            #define DECLARE_ASSIGN_OP(OP) \
            template<typename U> \
            varying_impl& operator OP##=(const varying_impl<U>& other);\
            template<typename U> \
            varying_impl& operator OP##=(const U& other);
            FOR_ALL_ASSIGNABLE_OP(DECLARE_ASSIGN_OP)
            #undef DECLARE_ASSIGN_OP
            
            template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
            varying_reference<std::remove_reference_t<decltype(*std::declval<U>())>> operator*()
            {
                return { *this };
            }
            
            template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
            varying_reference<std::remove_reference_t<decltype(*std::declval<U>())>> operator*() const
            {
                return { *this };
            }
            
            template<typename U, typename V = T, typename = std::enable_if_t<std::is_pointer_v<V>>>
            varying_reference<std::remove_reference_t<decltype(*std::declval<V>())>> operator[](const varying_impl<U>& index);
            
            template<typename U, typename V = T, typename = std::enable_if_t<std::is_pointer_v<V>>>
            varying_reference<std::remove_reference_t<decltype(*std::declval<V>())>> operator[](const varying_impl<U>& index) const;
            
            template<typename U, typename V = T, typename = std::enable_if_t<std::is_pointer_v<V>>>
            varying_reference<std::remove_reference_t<decltype(*std::declval<V>())>> operator[](const U& index);
            
            template<typename U, typename V = T, typename = std::enable_if_t<std::is_pointer_v<V>>>
            varying_reference<std::remove_reference_t<decltype(*std::declval<V>())>> operator[](const U& index) const;

            // Implementation detail but it is easier for this PoC to have this public
            std::array<T, LANE_SIZE> _values;

            varying_impl(Private token, const std::array<T, LANE_SIZE>& values) :
                _values{values} {}
        };
        
        template<typename T>
        struct varying_reference
        {
            varying_impl<T*> pointer;
            
            // Read from reference
            operator varying_impl<T>();
            
            // Write to reference
            template<typename U>
            requires std::convertible_to<U, T>
            varying_reference& operator=(const varying_impl<U>& other);
        };

        template<std::size_t... I>
        std::array<bool, LANE_SIZE> all_true(std::index_sequence<I...>)
        {
            return {
                (I, true)...
            };
        }
    }

    template<typename T, bool is_varying = true>
    using varying = std::conditional_t<is_varying, detail::varying_impl<T>, T>;
    
    template<typename T>
    using uniform = T;

    using mask_t = varying<bool>;

    thread_local mask_t _current_mask(detail::Private{}, detail::all_true(std::make_index_sequence<detail::LANE_SIZE>{}));

    namespace detail
    {
        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>
        varying_impl<T>::varying_impl():
            _values{} {}

        template<typename T, typename U, std::size_t... I>
        std::array<T, LANE_SIZE> create_values_with_mask_impl(const varying_impl<U>& other, std::index_sequence<I...>)
        {
            return {
                (_current_mask._values[I] ? static_cast<T>(other._values[I]) : T{})...
            };
        }

        template<typename T, typename U, std::size_t... I>
        std::array<T, LANE_SIZE> create_values_with_mask_impl(const U& other, std::index_sequence<I...>)
        {
            return {
                (_current_mask._values[I] ? static_cast<T>(other) : T{})...
            };
        }

        template<typename T, typename U>
        std::array<T, LANE_SIZE> create_values_with_mask(const varying_impl<U>& other)
        {
            return create_values_with_mask_impl<T>(other, std::make_index_sequence<LANE_SIZE>{});
        }

        template<typename T, typename U>
        std::array<T, LANE_SIZE> create_values_with_mask(const U& other)
        {
            return create_values_with_mask_impl<T>(other, std::make_index_sequence<LANE_SIZE>{});
        }

        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>

        template<typename U>
        requires std::convertible_to<U, T>
        varying_impl<T>::varying_impl(const varying_impl<U>& other):
            varying_impl(Private{}, {
                create_values_with_mask<T>(other)
            }) {}

        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>

        template<typename U>
        requires std::convertible_to<U, T>
        varying_impl<T>::varying_impl(const U& other):
            varying_impl(Private{}, {
                create_values_with_mask<T>(other)
            }) {}

        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>

        template<typename U>
        requires std::constructible_from<T, U>
                 && (!std::convertible_to<U, T>)
        varying_impl<T>::varying_impl(const varying_impl<U>& other):
            varying_impl(Private{}, {
                create_values_with_mask<T>(other)
            }) {}

        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>

        template<typename U>
        requires std::constructible_from<T, U>
                 && (!std::convertible_to<U, T>)
        varying_impl<T>::varying_impl(const U& other):
            varying_impl(Private{}, {
                create_values_with_mask<T>(other)
            }) {}

        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>
        varying_impl<T>::varying_impl(const varying_impl<T>& other):
            varying_impl(Private{}, {
                create_values_with_mask<T>(other)
            }) {}

        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>
        varying_impl<T>::varying_impl(varying_impl<T>&& other):
            varying_impl(Private{}, {
                create_values_with_mask<T>(other)
            }) {}
            

        template<typename T, typename U, std::size_t... I>
        void write_in_place_with_mask(std::array<T, LANE_SIZE>& self, const std::array<U, LANE_SIZE>& other,
                                      std::index_sequence<I...>)
        {
            (
                [&]()
                {
                    if(_current_mask._values[I])
                        self[I] = other[I];
                }(), ...
            );
        }

        template<typename T, typename U, std::size_t... I>
        void write_in_place_with_mask(std::array<T, LANE_SIZE>& self, const U& other, std::index_sequence<I...>)
        {
            (
                [&]()
                {
                    if(_current_mask._values[I])
                        self[I] = other;
                }(), ...
            );
        }

        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>

        template<typename U>
        requires std::convertible_to<U, T>
        varying_impl<T>& varying_impl<T>::operator=(const varying_impl<U>& other)
        {
            write_in_place_with_mask(_values, other._values, std::make_index_sequence<LANE_SIZE>{});
            return *this;
        }

        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>

        template<typename U>
        requires std::convertible_to<U, T>
        varying_impl<T>& varying_impl<T>::operator=(const U& other)
        {
            write_in_place_with_mask(_values, other, std::make_index_sequence<LANE_SIZE>{});
            return *this;
        }
        
        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>
        varying_impl<T>& varying_impl<T>::operator=(const varying_impl<T>& other)
        {
            write_in_place_with_mask(_values, other._values, std::make_index_sequence<LANE_SIZE>{});
            return *this;
        }
        
        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>
        varying_impl<T>& varying_impl<T>::operator=(varying_impl<T>&& other)
        {
            write_in_place_with_mask(_values, other._values, std::make_index_sequence<LANE_SIZE>{});
            return *this;
        }

        #define DEFINE_ASSIGN_OP(OP) \
        template<typename T> \
        requires std::default_initializable<T> \
                 && std::copyable<T> \
        template<typename U> \
        varying_impl<T>& varying_impl<T>::operator OP##=(const varying_impl<U>& other) \
        {\
            auto helper = []<typename T2, typename U2, std::size_t... I>(std::array<T, LANE_SIZE>& self, const std::array<U, LANE_SIZE>& other, std::index_sequence<I...>) \
            {\
                ( \
                    [&]() \
                    {\
                        if(_current_mask._values[I]) \
                            self[I] OP##= other[I]; \
                    }(), ...\
                );\
            };\
            helper(_values, other._values, std::make_index_sequence<LANE_SIZE>{});\
        }\
        template<typename T> \
        requires std::default_initializable<T> \
                 && std::copyable<T> \
        template<typename U> \
        varying_impl<T>& varying_impl<T>::operator OP##=(const U& other) \
        {\
            auto helper = []<typename T2, typename U2, std::size_t... I>(std::array<T, LANE_SIZE>& self, const U& other, std::index_sequence<I...>) \
            {\
                ( \
                    [&]() \
                    {\
                        if(_current_mask._values[I]) \
                            self[I] OP##= other; \
                    }(), ...\
                );\
            };\
            helper(_values, other, std::make_index_sequence<LANE_SIZE>{});\
        }
        FOR_ALL_ASSIGNABLE_OP(DEFINE_ASSIGN_OP)

        #undef DEFINE_ASSIGN_OP

        #define DEFINE_OP(OP) \
        template<typename LHS, typename RHS> \
        varying_impl<decltype(std::declval<LHS>() OP std::declval<RHS>())> operator OP(const varying_impl<LHS>& lhs, const varying_impl<RHS>& rhs) \
        { \
            using Return = decltype(std::declval<LHS>() OP std::declval<RHS>()); \
            \
            auto helper = []<std::size_t... I>(const std::array<LHS, LANE_SIZE>& lhs, const std::array<RHS, LANE_SIZE>& rhs, std::index_sequence<I...>) \
            { \
                return std::array<Return, LANE_SIZE> \
                    { \
                        (_current_mask._values[I] ? lhs[I] OP rhs[I] : Return{})... \
                    }; \
            }; \
            \
            return varying_impl<Return>(Private{}, helper(lhs._values, rhs._values, std::make_index_sequence<LANE_SIZE>{})); \
        } \
        template<typename LHS, typename RHS> \
        varying_impl<decltype(std::declval<LHS>() OP std::declval<RHS>())> operator OP(const varying_impl<LHS>& lhs, const RHS& rhs) \
        { \
            using Return = decltype(std::declval<LHS>() OP std::declval<RHS>()); \
            \
            auto helper = []<std::size_t... I>(const std::array<LHS, LANE_SIZE>& lhs, const RHS& rhs, std::index_sequence<I...>) \
            { \
                return std::array<Return, LANE_SIZE> \
                    { \
                        (_current_mask._values[I] ? lhs[I] OP rhs : Return{})... \
                    }; \
            }; \
            \
            return varying_impl<Return>(Private{}, helper(lhs._values, rhs, std::make_index_sequence<LANE_SIZE>{})); \
        } \
        template<typename LHS, typename RHS> \
        varying_impl<decltype(std::declval<LHS>() OP std::declval<RHS>())> operator OP(const LHS& lhs, const varying_impl<RHS>& rhs) \
        { \
            using Return = decltype(std::declval<LHS>() OP std::declval<RHS>()); \
            \
            auto helper = []<std::size_t... I>(const LHS& lhs, const std::array<RHS, LANE_SIZE>& rhs, std::index_sequence<I...>) \
            { \
                return std::array<Return, LANE_SIZE> \
                    { \
                        (_current_mask._values[I] ? lhs OP rhs[I] : Return{})... \
                    }; \
            }; \
            \
            return varying_impl<Return>(Private{}, helper(lhs, rhs._values, std::make_index_sequence<LANE_SIZE>{})); \
        }

        FOR_ALL_OP(DEFINE_OP)

        #undef DEFINE_OP

        #undef FOR_ALL_OP
        #undef FOR_ALL_ASSIGNABLE_OP

        #define FOR_ALL_UNARY_OP(MACRO) \
        MACRO(+)                        \
        MACRO(-)                        \
        MACRO(!)                        \
        MACRO(~)

        #define DEFINE_UNARY_OP(OP) \
        template<typename T>\
        varying_impl<std::remove_reference_t<decltype(OP std::declval<T>())>> operator OP(const varying_impl<T>& self)\
        {\
            using Return = std::remove_reference_t<decltype(OP std::declval<T>())>;\
            auto helper = [&]<std::size_t... I>(std::index_sequence<I...>)\
            {\
                return std::array<Return, LANE_SIZE>{\
                    (_current_mask._values[I] ? OP self._values[I] : Return{})...\
                };\
            };\
            return varying_impl<Return>(Private{}, helper(std::make_index_sequence<LANE_SIZE>{}));\
        };
        
        FOR_ALL_UNARY_OP(DEFINE_UNARY_OP)
        #undef DEFINE_UNARY_OP
        #undef FOR_ALL_UNARY_OP
        
        template<typename T>
        std::ostream& operator<<(std::ostream& out, const varying_impl<T>& v)
        {
            out << "{ ";
            for(int i = 0; i < LANE_SIZE; ++i)
            {
                if(!_current_mask._values[i])
                    out << "(";
                out << v._values[i];
                if(!_current_mask._values[i])
                    out << ")";
                if(i == LANE_SIZE - 1)
                    out << " }";
                else
                    out << ", ";
            }
            return out;
        }

        template<typename T>
        varying_reference<T>::operator varying_impl<T>()
        {
            auto helper = [&]<std::size_t... I>(std::index_sequence<I...>)
            {
                return std::array<T, LANE_SIZE>{
                    (_current_mask._values[I] ? *pointer._values[I]: T{} )...
                };
            };
            return varying_impl<T>(Private{}, helper(std::make_index_sequence<LANE_SIZE>{}));
        }

        template<typename T>
        template<typename U>
        requires std::convertible_to<U, T>
        varying_reference<T>& varying_reference<T>::operator=(const varying_impl<U>& other)
        {
            auto helper = [&]<std::size_t... I>(std::index_sequence<I...>)
            {
                (
                    [&]()
                    {
                        if(_current_mask._values[I])
                            *pointer._values[I] = other._values[I];
                    }(),...
                );
            };
            helper(std::make_index_sequence<LANE_SIZE>{});
            return *this;
        }
        
        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>
        template<typename U, typename V, typename>
        varying_reference<std::remove_reference_t<decltype(*std::declval<V>())>>
        varying_impl<T>::operator[](const varying_impl<U>& index)
        {
            return *(*this + index);
        }
        
        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>
        template<typename U, typename V, typename>
        varying_reference<std::remove_reference_t<decltype(*std::declval<V>())>>
        varying_impl<T>::operator[](const varying_impl<U>& index) const
        {
            return *(*this + index);
        }
        
        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>
        template<typename U, typename V, typename>
        varying_reference<std::remove_reference_t<decltype(*std::declval<V>())>>
        varying_impl<T>::operator[](const U& index)
        {
            return *(*this + index);
        }
        
        template<typename T>
        requires std::default_initializable<T>
                 && std::copyable<T>
        template<typename U, typename V, typename>
        varying_reference<std::remove_reference_t<decltype(*std::declval<V>())>>
        varying_impl<T>::operator[](const U& index) const
        {
            return *(*this + index);
        }
    }
}

#endif // VARYING_HPP
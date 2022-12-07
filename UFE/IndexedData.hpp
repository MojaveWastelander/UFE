#pragma once
#include <type_traits>
#include <any>
#include <functional>
#include <iomanip>
#include <iostream>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

template<class T, class F>
inline std::pair<const std::type_index, std::function<void(std::any const&)>>
to_any_visitor(F const& f)
{
    return {
        std::type_index(typeid(T)),
        [g = f](std::any const& a)
        {
            if constexpr (std::is_void_v<T>)
                g();
            else
                g(std::any_cast<T const&>(a));
        }
    };
}

static std::unordered_map<
    std::type_index, std::function<void(std::any const&)>>
    any_visitor{
        to_any_visitor<void>([] { std::cout << "{}"; }),
    // ... add more handlers for your types ...
};

inline void process(const std::any& a)
{
    if (const auto it = any_visitor.find(std::type_index(a.type()));
        it != any_visitor.cend()) {
        it->second(a);
    }
    else {
        std::cout << "Unregistered type " << std::quoted(a.type().name()) << '\n';
    }
}

template<class T, class F>
inline void register_any_visitor(F const& f)
{
    std::cout << "Register visitor for type "
        << std::quoted(typeid(T).name()) << '\n';
    any_visitor.insert(to_any_visitor<T>(f));
}

class FileReader;

template <typename T>
struct IndexedData
{
    size_t m_offset;
    T m_data;
};
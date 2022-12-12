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
#include <nlohmann/json.hpp>

template<class T, class F>
inline std::pair<const std::type_index, std::function<nlohmann::ordered_json(std::any const& a)>>
to_any_visitor(F const& f)
{
    return std::make_pair(
        std::type_index(typeid(T)),
        [g = f](std::any const& a)
        {
            if constexpr (std::is_void_v<T>)
                return g();
            else
                return g(std::any_cast<T const&>(a));
        }
    );
}

using types_functions_map = std::unordered_map<
    std::type_index, std::function<nlohmann::ordered_json(std::any const&)>>;


//inline void process(const std::any& a)
//{
//    if (const auto it = any_visitor.find(std::type_index(a.type()));
//        it != any_visitor.cend()) {
//        it->second(a);
//    }
//    else {
//        std::cout << "Unregistered type " << std::quoted(a.type().name()) << '\n';
//    }
//}

template<class T, class F>
inline void register_any_visitor(types_functions_map& map, F const& f)
{
    std::cout << "Register visitor for type "
        << std::quoted(typeid(T).name()) << '\n';
    map.insert(to_any_visitor<T>(f));
}

class BinaryFileParser;

template <typename T>
struct IndexedData
{
    size_t m_offset;
    T m_data;
};
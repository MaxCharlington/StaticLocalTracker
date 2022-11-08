#pragma once

#include <cassert>
#include <mutex>
#include <shared_mutex>
#include <source_location>
#include <string_view>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>

#include "deps/helpers/strong_type.hpp"

template <typename T>
struct std::hash<StrongType<std::string_view, T>>
{
    constexpr std::size_t operator()(const StrongType<std::string_view, T> strong) const noexcept
    {
        return std::hash<std::string_view>{}(strong.get());
    }
};

namespace StaticLocalTracker
{

using VarNameTag = struct Func;
using FuncNameTag = struct Func;
using FileNameTag = struct File;

using VarName = StrongType<std::string_view, VarNameTag>;
using FuncName = StrongType<std::string_view, FuncNameTag>;
using FileName = StrongType<std::string_view, FileNameTag>;

class static_local
{
private:
    void* m_ptr;
    const std::type_info* m_type_info;

public:
    static_local() = default;

    template<typename T>
    static_local(T& var) :
        m_ptr{&var},
        m_type_info{&typeid(T)}
    {}

    static_local(const static_local&) = default;

    template<typename T>
    T* get()
    {
        if (typeid(T) == *m_type_info)
            return reinterpret_cast<T*>(m_ptr);
        else
            return nullptr;
    }
};

using static_locals_t = std::unordered_map<VarName, static_local>;
using func_to_static_locals_t = std::unordered_map<FuncName, static_locals_t>;
using file_to_funcs_t = std::unordered_map<FileName, func_to_static_locals_t>;

inline file_to_funcs_t statics{};
inline std::shared_mutex mtx;

template<typename T>
void track(T& var, VarName var_name, const std::source_location place = std::source_location::current())
{
    std::unique_lock lk{mtx};

    auto file_name = FileName{place.file_name()};
    auto func_name = FuncName{place.function_name()};

    statics[file_name][func_name].insert({var_name, var});
}

template<typename T>
T* get(FileName file_name, FuncName func_name, VarName var_name)
{
    std::shared_lock lk{mtx};

    if (statics.contains(file_name) and statics.at(file_name).contains(func_name) and statics.at(file_name).at(func_name).contains(var_name))
        return statics.at(file_name).at(func_name).at(var_name).template get<T>();
    return nullptr;
}

template<typename T>
T* get(FuncName func_name, VarName var_name)
{
    std::shared_lock lk{mtx};

    for (const auto& [file_name, funcs] : statics)
        if (statics.at(file_name).contains(func_name) and statics.at(file_name).at(func_name).contains(var_name))
            return statics.at(file_name).at(func_name).at(var_name).template get<T>();
    return nullptr;
}

template<typename T>
T* get(VarName var_name)
{
    std::shared_lock lk{mtx};

    for (const auto& [file_name, funcs] : statics)
        for (const auto& [func_name, vars] : statics.at(file_name))
            if (statics.at(file_name).at(func_name).contains(var_name))
                return statics.at(file_name).at(func_name).at(var_name).template get<T>();
    return nullptr;
}

} // namespace StaticLocalTracker


// Adds static local variable to global tracking
#define STATIC_LOCAL_TRACK(var) static auto _tracker = []{                                                             \
    StaticLocalTracker::track<std::decay_t<decltype(var)>>(var, StaticLocalTracker::VarName{std::string_view{#var}});  \
    return 0;                                                                                                          \
}()

// Copies static local by <type> and <name> to variable
#define STATIC_LOCAL_COPY(type, name) []() -> type {                                                  \
    static_assert(std::is_copy_constructible_v<type>, "type should be copy constructible");           \
    auto* ptr = StaticLocalTracker::get<type>(StaticLocalTracker::VarName{std::string_view{#name}});  \
    assert(ptr != nullptr);                                                                           \
    return *ptr;                                                                                      \
}()

// Copies static local to variable by out parameter
#define STATIC_LOCAL_COPY_TO(var) { \
    auto* ptr = StaticLocalTracker::get<decltype(var)>(StaticLocalTracker::VarName{std::string_view{#var}});  \
    assert(ptr != nullptr);                                                                                   \
    var = *ptr;                                                                                               \
}

// Obtains reference to static local object by <type> and <name>
#define STATIC_LOCAL_GET_REF(type, name) []() -> type& {                                              \
    auto* ptr = StaticLocalTracker::get<type>(StaticLocalTracker::VarName{std::string_view{#name}});  \
    assert(ptr != nullptr);                                                                           \
    return *ptr;                                                                                      \
}()

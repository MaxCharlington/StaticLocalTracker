#include <cassert>
#include <source_location>
#include <string_view>
#include <typeinfo>
#include <unordered_map>

template<typename T, typename Tag>
class StrongType
{
public:
    explicit StrongType(const T& val) : m_val{val} {}
    T get() const { return m_val; }
    bool operator==(const StrongType<T, Tag>& other) const { return this->m_val == other.m_val; }

private:
    T m_val;
};

template <typename T>
struct std::hash<StrongType<std::string_view, T>>
{
    constexpr std::size_t operator()(const StrongType<std::string_view, T> strong) const noexcept
    {
        return std::hash<std::string_view>{}(strong.get());
    }
};

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
        return nullptr;
    }

    template<typename T>
    static void track(T&, VarName, const std::source_location = std::source_location::current());
    
    template<typename T>
    static T* get(FileName, FuncName, VarName);

    template<typename T>
    static T* get(FuncName, VarName);

    template<typename T>
    static T* get(VarName);
};

using static_locals_t = std::unordered_map<VarName, static_local>;
using func_to_static_locals_t = std::unordered_map<FuncName, static_locals_t>;
using file_to_funcs_t = std::unordered_map<FileName, func_to_static_locals_t>;

inline file_to_funcs_t statics{};

template<typename T>
void static_local::track(T& var, VarName var_name, const std::source_location place)
{
    auto file_name = FileName{place.file_name()};
    auto func_name = FuncName{place.function_name()};

    statics[file_name][func_name].insert({var_name, var});
}

template<typename T>
T* static_local::get(FileName file_name, FuncName func_name, VarName var_name)
{
    if (statics.contains(file_name))
    {
        if (statics.at(file_name).contains(func_name))
        {
            if (statics.at(file_name).at(func_name).contains(var_name))
            {
                return statics.at(file_name).at(func_name).at(var_name).template get<T>();
            }
        }
    }
    return nullptr;
}

template<typename T>
T* static_local::get(FuncName func_name, VarName var_name)
{
    for (const auto& [file_name, funcs] : statics)
    {
        if (statics.at(file_name).contains(func_name))
        {
            if (statics.at(file_name).at(func_name).contains(var_name))
            {
                return statics.at(file_name).at(func_name).at(var_name).template get<T>();
            }
        }
    }
    return nullptr;
}

template<typename T>
T* static_local::get(VarName var_name)
{
    for (const auto& [file_name, funcs] : statics)
    {
        for (const auto& [func_name, vars] : statics.at(file_name))
        {
            if (statics.at(file_name).at(func_name).contains(var_name))
            {
                return statics.at(file_name).at(func_name).at(var_name).template get<T>();
            }
        }
    }
    return nullptr;
}

#define STATIC_LOCAL_TRACK(x) static_local::track<std::decay_t<decltype(x)>>(x, VarName{std::string_view{#x}})
#define STATIC_LOCAL_GET(x) { \
    auto* ptr = static_local::get<decltype(x)>(VarName{std::string_view{#x}}); \
    assert(ptr != nullptr); \
    x = *ptr; \
}


void f() {
    static int some_num = 5;
    STATIC_LOCAL_TRACK(some_num);
}


int main() {
    f();

    // auto* some_num_ptr = static_local::get<int>(VarName{"some_num"});
    // if (some_num_ptr == nullptr)
    //     return 1;
    // else
    //     return *some_num_ptr;

        
    int some_num;
    STATIC_LOCAL_GET(some_num);

    return some_num;
}

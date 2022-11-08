
#include <static_local_tracker.hpp>

struct S
{
    S() = default;
    S(int a_) : a{a_} {}
    S(const S&) = delete;
    S& operator=(const S&) = delete;

    int a;
};

void f()
{
    static S s(5);
    STATIC_LOCAL_TRACK(s);
}

int main()
{
    f();
    S& s = STATIC_LOCAL_GET_REF(S, s);
    return s.a;
}

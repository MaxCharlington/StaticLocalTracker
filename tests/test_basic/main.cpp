#include <static_local_tracker.hpp>

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

    STATIC_LOCAL_COPY_TO(some_num);
    some_num = STATIC_LOCAL_COPY(int, some_num);
    int& some_num_ref = STATIC_LOCAL_GET_REF(int, some_num);

    return some_num_ref;
}

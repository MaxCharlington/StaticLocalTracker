#include <static_local_tracker.hpp>

void f() {
    static int some_num = 5;
    STATIC_LOCAL_TRACK(some_num);
}


int main() {
    f();

    int some_num;

    STATIC_LOCAL_COPY_TO(some_num);
    some_num = STATIC_LOCAL_COPY(int, some_num);
    int& some_num_ref = STATIC_LOCAL_GET_REF(int, some_num);

    return some_num_ref;
}

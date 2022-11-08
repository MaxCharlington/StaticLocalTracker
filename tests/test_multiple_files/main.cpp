#include <static_local_tracker.hpp>

void f();

int main() {
    f();

    int some_num;
    STATIC_LOCAL_COPY_TO(some_num);

    return some_num;
}

#include <static_local_tracker.hpp>

void f() {
    static int some_num = 5;
    STATIC_LOCAL_TRACK(some_num);
}

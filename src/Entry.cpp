#include <assert.h>
#include <uvw.hpp>
#include <Windows.h>

static void thread_cb(std::shared_ptr<void> arg) {
    printf("Hello World!\n");
}


int main() {
    auto loop = uvw::Loop::getDefault();
    auto thread = loop->resource<uvw::Thread>(thread_cb);
    thread->run();
    thread->join();

    return 0;
}

#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>


int main(){
    bulk::thread::environment env;
    env.spawn(env.available_processors(), [](auto& world) {
        auto s = world.rank();
        auto p = world.active_processors();

        world.log("Hello world from processor %d / %d!", s, p);
    });
}

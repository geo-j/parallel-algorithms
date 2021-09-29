#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>


int main(){
    bulk::thread::environment env;
    env.spawn(env.available_processors(), [](auto& world) {
        auto raw_queue = bulk::queue<int>(world);
        raw_queue(world.next_rank()).send(1);
        raw_queue(world.next_rank()).send(2);
        raw_queue(world.next_rank()).send(123);

        auto tuple_queue = bulk::queue<int, int, int>(world);
        tuple_queue(world.next_rank()).send(1, 2, 3);
        tuple_queue(world.next_rank()).send(4, 5, 6);

        world.sync();

        // read queue
        for (auto x : raw_queue) {
            world.log("the first queue received a message: %d", x);
        }

        for (auto [i, j, k] : tuple_queue) {
            world.log("the second queue received a tuple: (%d, %d, %d)", i, j, k);
        }
            });
}

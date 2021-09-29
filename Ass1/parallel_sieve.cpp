#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>

using namespace std;

bool isPrime(int n){
    if (n < 2){
        return false;
    }

    if (n == 2 || n == 3){
        return true;
    }

    for (int i = 2; i * i <= n; i ++){
        if (n % i == 0)
            return false;
    }

    return true;
}

int main(){
    int n, x = 2;
    cin >> n;

    bulk::thread::environment env;
    env.spawn(env.available_processors(), [&n, &x](auto& world) {
        // init local processors
        auto pid = world.rank(); // local processor ID
        auto p = world.active_processors();

        // create cyclically partitioned array
        auto cp = bulk::cyclic_partitioning<1>({n}, {p});
        auto cyclic_local_size = cp.local_count(world.rank());
        auto all_numbers = bulk::coarray<int>(world, cyclic_local_size);

        // create blockly partitioned boolean array
        auto bp = bulk::block_partitioning<1>({n}, {p});
        auto block_local_size = bp.local_count(world.rank());
        auto primes = bulk::coarray<bool>(world, block_local_size); // boolean array holding 1 if a number is prime, 0 otherwise

        // fill all numbers array with numbers from 2 to n
        world.log("cyclic size %d", cyclic_local_size);
        for (int i = 0; i < cyclic_local_size; i ++){
            world.log("processor %d, index %d", pid, i);
            all_numbers(pid)[i] = x;
            x ++;
        }

        world.log("block size %d", block_local_size);
        for (int i = 0; i < block_local_size; i ++){
            world.log("processor %d, index %d", pid, i);
            primes(pid)[i] = 1;
        }

        world.sync();

        for (auto x : primes){
            world.log("%d", x);
        }
    });
}

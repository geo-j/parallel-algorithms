#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>

using namespace std;

int x = 2;

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
    int n;
    cin >> n;

    bulk::thread::environment env;
    env.spawn(env.available_processors(), [&n](auto& world) {
        // init local processors
        auto pid = world.rank(); // local processor ID
        auto p = world.active_processors();


        // create cyclically partitioned array
        auto cp = bulk::cyclic_partitioning<1>({n}, {p});
        auto cyclic_local_size = cp.local_count(world.rank());
        auto primes = bulk::coarray<bool>(world, cyclic_local_size);

        // create blockly partitioned boolean array
        // auto bp = bulk::block_partitioning<1>({n}, {p});
        // auto block_local_size = bp.local_count(world.rank());
        // auto primes = bulk::coarray<bool>(world, block_local_size); // boolean array holding 1 if a number is prime, 0 otherwise

        // fill all numbers array with numbers from 2 to n
        // world.log("cyclic size %d", cyclic_local_size);
        // for (int i = 0; i < cyclic_local_size; i ++){
        //     world.log("processor %d, index %d, number %d", pid, i, x);
        //     // all_numbers(pid)[i] = x(world.next_rank()).get().value();
        //     // x ++;
        //     world.sync();
        // }

        // world.sync();
        // world.log("block size %d", block_local_size);
        for (int i = 0; i < cyclic_local_size; i ++){
            world.log("processor %d, index %d, number %d", pid, i, pid + i * p + 1);
            primes[i] = true;
        }

        world.sync();

        for (int i = 0; i < cyclic_local_size; i ++){
            int current = pid + i * p + 1;
            world.log("processor %d, current %d", pid, current);
            if (primes[i] || isPrime(current)){
                world.log("\tprocessor %d, prime found %d", pid, current);
                for (int j = i + 1; j < cyclic_local_size; j ++) {
                    world.log("\tprocessor %d, trying %d for division", pid, pid + j * p + 1);
                    if ((pid + j * p + 1) % current == 0){
                        world.log("\tmark processor %d, index %d", pid * j, j);
                        primes[j] = false;
                    }
                }
            } else {
                primes[i] = false;
            }
        }

        world.sync();

        for (int i = 0; i < cyclic_local_size; i ++){
            world.log("number %d, prime? %d", pid + i * p + 1, primes[i]);
        }
    });
}

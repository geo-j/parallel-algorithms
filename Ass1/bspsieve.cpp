#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>
#include <string>
#include <chrono>

using namespace std;

// function that sends the locally found primes to all the other processors
void put_primes(int prime, int pid, int p, bulk::queue<int> &q) {
    for (int i = 0; i < p; i ++) {
        if (i != pid) {
            q(i).send(prime);
        }
    }
}

int main(int argc, char* argv[]) {
    int n, p = -1;

    for (int i = 1; i < argc; i ++) {
        string arg = argv[i];
        if (arg == "-n") {
            n = stoi(argv[++ i]);
        } else if (arg == "-p") {
            p = stoi(argv[++ i]);
        } else {
            cerr << "wrong arguments";
        }
    }

    const auto start = chrono::steady_clock::now();

    bulk::thread::environment env;
    env.spawn(env.available_processors(), [&n, &p](auto& world) {
        // init local processors
        auto pid = world.rank(); // local processor ID

        if (p == -1)    // if the number of processors hasn't been specified, use all by
            p = world.active_processors();

        // create cyclically partitioned array
        auto cp = bulk::cyclic_partitioning<1>({n}, {p});
        auto cyclic_local_size = cp.local_count(world.rank());
        auto primes = bulk::coarray<bool>(world, cyclic_local_size);

        // init boolean primes array with true, except for the first element 1
        world.log("==== Superstep 1");
        for (int i = 0; i < cyclic_local_size; i ++){
            int current = pid + i * p + 1;
            // world.log("\tprocessor %d, index %d, number %d", pid, i, current);
            if (current == 1)
                primes[i] = false;
            else
                primes[i] = true;
        }

        world.log("Sync-ing...");
        world.sync();

        // create an array with which to communicate potential primes for each of the processors
        auto local_primes = bulk::queue<int>(world);

        world.log("==== Superstep 2");
        // iterate through all the local numbers
        for (int i = 0; i < cyclic_local_size; i ++){
            int current = pid + i * p + 1;
            // world.log("\tprocessor %d, current %d", pid, current);

            // if the current number is marked as prime, communicate it to the other processors and sieve its multiples
            if (primes[i]){
                put_primes(current, pid, p, local_primes);

                for (int j = i + 1; j < cyclic_local_size; j ++){
                    int multiple = pid + j * p + 1;
                    if (multiple % current == 0){
                        // world.log("\t\tprocessor %d, non-prime %d found", pid, multiple);
                        primes[j] = false;
                    }
                }
            }
            
        }

        world.log("Sync-ing...");
        world.sync();

        world.log("==== Superstep 3");
        // for each of the received primes from the other processors, sieve any marked-as-prime local numbers
        for (auto prime : local_primes) {
            // world.log("\tprocessor %d received prime %d", pid, prime);

            for (int j = 0; j < cyclic_local_size; j ++) {
                int current = pid + j * p + 1;
                if (current % prime == 0 && primes[j]) {
                    // world.log("\t\tprocessor %d, non-prime %d found", pid, current);
                    primes[j] = false;
                }
            }
        }

        world.log("Sync-ing...");
        world.sync();
        for (int i = 0; i < cyclic_local_size; i ++){
            if (primes[i])
                world.log("number %d, prime? %d", pid + i * p + 1, primes[i]);
        }
    });

    const auto end = chrono::steady_clock::now();
    cout << "It took " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms";
}

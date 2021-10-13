#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>
#include <string>
#include <chrono>
#include <fstream>
#include <vector>
#include <math.h>

using namespace std;

int gcd(int a, int b) {
   if (a == 0 || b == 0)
        return 0;
   else if (a == b)
        return a;
   else if (a > b)
        return gcd(a - b, b);
   else return gcd(a, b - a);
}

// function that sends the number to all the other processors
void put_numbers_all(int number, int pid, int p, bulk::queue<int> &q) {
    for (int i = 0; i < p; i ++) {
        if (i != pid) { 
            q(i).send(number);
        }
    }
}

// function that sends the locally found primes to only the processors that might have multiples of it
void put_primes(int prime, int n, int p, bulk::queue<int> &q) {
    for (int i = prime * 2; i < n; i += prime) {
        q((i - 1) % p).send(i);
    }
}

int main(int argc, char* argv[]) {
    bulk::thread::environment env;
    size_t n, p = env.available_processors();
    ofstream f;
    vector<int> flops = vector<int>(p);

    for (int i = 1; i < argc; i ++) {
        string arg = argv[i];
        if (arg == "-n") {
            n = static_cast<size_t>(stoi(argv[++ i]));
        } else if (arg == "-p") {
            p = static_cast<size_t>(stoi(argv[++ i]));
        } else {
            cerr << "wrong arguments";
        }
    }

    const auto start = chrono::steady_clock::now();

    env.spawn(p, [&n, &p, &flops](auto& world) {
        // init local processors
        auto pid = world.rank(); // local processor ID
        
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

        int flop = 0;

        world.log("==== Superstep 2");
        // iterate through all the local numbers
        for (int i = 0; i < cyclic_local_size; i ++){
            // world.log("\tprocessor %d, current %d", pid, current);

            // if the current number is marked as prime, communicate it to the other processors and sieve its multiples
            if (primes[i]){
                int current = pid + i * p + 1;
                put_primes(current, n, p, local_primes);

                // sieving will be done with a step-size of the current prime / gcd(current, p)
                int d = gcd(current, p);
                int step = current / d;

                for (int j = i + step; j < cyclic_local_size; j += step){
                    // int multiple = pid + j * p + 1;
                    // if (multiple % current == 0){
                        // world.log("\t\tprocessor %d, step %d, non-prime %d found", pid, step, pid + j * p + 1);
                        primes[j] = false;
                        flop ++;
                    // }
                }
            }
            
        }

        world.log("Sync-ing...");
        world.sync();

        world.log("==== Superstep 3");
        // for each of the received primes from the other processors, sieve any marked-as-prime local numbers
        for (auto prime : local_primes) {
            // world.log("\tprocessor %d received prime %d", pid, prime);

            // start from the first multiple of the prime in the current boolean array
            for (int j = ceil(prime / p); j < cyclic_local_size; j += prime) {
                // int current = pid + j * p + 1;
                // world.log("primes[%d] = %d", j, current);
                if (primes[j]) {
                    // world.log("\t\tprocessor %d, non-prime %d found", pid, current);
                    primes[j] = false;
                    flop ++;
                }
            }
        }

        world.log("Sync-ing...");
        world.sync();
        for (int i = 0; i < cyclic_local_size; i ++){
            if (primes[i])
                world.log("number %d, prime? %d", pid + i * p + 1, primes[i]);
        }

        flops[pid] = flop;
    });

    const auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    cout << "It took " << duration << " ms and " << endl;

    f.open("results.csv", ios_base::app);
    for (int i = 0; i < p; i ++) {
        f << n << ',' << p  << ',' << duration << ',' << i << ',' << flops.at(i) << endl;
    }
    f.close();
}

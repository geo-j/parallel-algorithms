#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>
#include <string>
#include <chrono>
#include <fstream>
#include <vector>
#include "utils.hpp"

using namespace std;




void sieve_local_prime_sums(size_t prime1, size_t prime2, size_t n, int pid, int p, size_t &flop, bulk::queue<int> &q, bulk::coarray<bool> &even_numbers,  bulk::world& world) {
    size_t sum = prime1 + prime2;
    size_t sum_processor = (sum - 1) % p;
    // world.log("\tprocessor %d: prime1 = %d, prime2 = %d", pid, prime1, prime2);

    flop += 4;

    // if the prime sum is in the current processor, sieve it; otherwise send it to the corresponding processor
    if (sum <= n && sum % 2 == 0) {
        flop ++;
        if (sum_processor == pid) {
            size_t k = number_to_index(sum, p, pid, flop);
            // world.log("\tfound local summed prime %d on index %d", sum, k);
            even_numbers[k] = true;
        }
        else {
            // world.log("\tsend summed prime %d to processor %d", sum, sum_processor);
            q(sum_processor).send(sum);
        }

    }
}

int main(int argc, char* argv[]) {
    bulk::thread::environment env;
    size_t n, p = env.available_processors();
    ofstream f;
    vector<size_t> flops = vector<size_t>(p);

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
    
        size_t flop = 0;


        // init boolean primes array with true, except for the first element 1
        // world.log("==== Superstep 1");
        for (size_t i = 0; i < cyclic_local_size; i ++){
            size_t current = index_to_number(i, pid, p, flop);

            flop ++;
            // world.log("\tprocessor %d, index %d, number %d", pid, i, current);
            if (current == 1)
                primes[i] = false;
            else
                primes[i] = true;
        }

        // world.log("Sync-ing...");
        world.sync();

        // create an array with which to communicate potential primes for each of the processors
        auto local_primes = bulk::queue<size_t>(world);

        // world.log("==== Superstep 2");
        // iterate through all the local numbers
        for (size_t i = 0; i < cyclic_local_size; i ++){
            flop ++;
            // world.log("\tprocessor %d, current %d", pid, current);

            // if the current number is marked as prime, communicate it to the other processors and sieve its multiples
            if (primes[i]){
                size_t current = index_to_number(i, pid, p, flop);
                flop ++;
                put_prime_multiples(current, n, p, local_primes, flop, world);
                // put_numbers_to_all(current, pid, p, local_primes);

                // sieving will be done with a step-size of the current prime / gcd(current, p)
                size_t d = gcd(current, p, flop);
                size_t step = current / d;

                flop += 2;

                for (size_t j = i + step; j < cyclic_local_size; j += step){
                    // int multiple = pid + j * p + 1;
                    // if (multiple % current == 0){
                        // world.log("\t\tprocessor %d, step %d, non-prime %d found", pid, step, pid + j * p + 1);
                        primes[j] = false;
                        flop ++;
                    // }
                }
            }
            
        }

        // world.log("Sync-ing...");
        world.sync();

        // world.log("==== Superstep 3");
        // for each of the received primes from the other processors, sieve any marked-as-prime local numbers
        for (auto prime : local_primes) {
            // world.log("\tprocessor %d received non-prime %d", pid, prime);

            flop ++;
            // start from the first multiple of the prime in the current boolean array
            for (size_t j = (prime - 1 - pid) / p; j < cyclic_local_size; j += prime) {
                // int current = pid + j * p + 1;
                // world.log("primes[%d] = %d", j, prime);
                flop ++;
                if (primes[j]) {
                    // world.log("\t\tprocessor %d, non-prime %d found", pid, prime);
                    primes[j] = false;
                }
            }
        }

        // world.log("Sync-ing...");
        world.sync();
        // for (int i = 0; i < cyclic_local_size; i ++){
        //     if (primes[i])
        //         world.log("number %d, prime? %d", pid + i * p + 1, primes[i]);
        // }

        // find Twin Primes
        // world.log("==== Superstep 4");
        // // send the local primes only to the processors that might possibly have its twin prime (which is a processor 2 units away from the current processor)
        // for (int i = 0; i < cyclic_local_size; i ++) {
        //     flop ++;
        //     if (primes[i]) {
        //         int current = index_to_number(i, pid, p, flop);
        //         local_primes((pid + 2) % p).send(current);

        //         flop += 2;
        //         // world.log("\tprocessor %d sends %d to processor %d hoping for prime %d", pid, current, (pid + 2) % p, (current - 1 - (pid + 2) % p) / p + 2);
        //     }
        // }
        // world.log("Sync-ing...");
        // world.sync();

        // world.log("==== Superstep 5");
        // for (auto prime : local_primes) {
        //     flop += 2;
        //     int i = number_to_index(prime, p, pid, flop);
        //     // world.log("\tprocessor %d trying %d and %d on index %d as twin primes", pid, prime, prime, i);
        //     if (primes[i]) {
        //         int current = index_to_number(i, pid, p, flop);
        //         flop ++;
        //         world.log("found twin primes (%d, %d)", prime, current);
        //     }
        // }

        // world.log("==== Superstep 6");

        // // Goldbach Conjecture
        // auto even_numbers = bulk::coarray<bool>(world, cyclic_local_size);

        // // init even numbers array with 0 (not sieved);
        // for (int i = 0; i < cyclic_local_size; i ++) {
        //     even_numbers[i] = false;
        // }

        // world.sync();

        // world.log("==== Superstep 7");

        // auto local_sums = bulk::queue<int>(world);

        // // send local primes to all other processors
        // for (int i = 0; i < cyclic_local_size; i ++) {
        //     if (primes[i]){
        //         int prime = index_to_number(i, pid, p, flop);
        //             // send prime to all other processors
        //         put_numbers_to_all(prime, pid, p, local_primes);
        //     }       
        // }

        // world.sync();

        // world.log("==== Superstep 8");

        // // sieve local prime sums, and send remote sums to their corresponding processor
        // for (int i = 0; i < cyclic_local_size; i ++) {
        //     flop ++;

        //     if (primes[i]) {
        //         int prime1 = index_to_number(i, pid, p, flop);

        //         for (int j = i; j < cyclic_local_size; j ++) {
        //             flop ++;

        //             if (primes[j]) {
        //                 // world.log("processor %d: i = %d, j = %d", pid,  i, j);
        //                 int prime2 = index_to_number(j, pid, p, flop);
        //                 // put_numbers_to_all(prime2, pid, p, local_primes);

        //                 sieve_local_prime_sums(prime1, prime2, n, pid, p, flop, local_sums, even_numbers, world);
        //             }
        //         }

        //         for (auto prime2 : local_primes) {
        //             // world.log("processor %d got prime %d: i = %d", pid, prime2, i);
        //             sieve_local_prime_sums(prime1, prime2, n, pid, p, flop, local_sums, even_numbers, world);
        //         }
        //     }
        // }

        // world.sync();

        // world.log("==== Superstep 8");

        // // sieve the remotely gotten sums
        // for (auto sum : local_sums) {
        //     int i = number_to_index(sum, p, pid, flop);
        //     // world.log("\tprocessor %d received summed prime %d on index %d", pid, sum, i);
        //     even_numbers[i] = true;
        // }

        // world.sync();

        // world.log("==== Superstep 9");

        // // count how many even numbers larger than 2 are sums of primes
        // int sum = 0;

        // for (int i = 0; i < cyclic_local_size; i ++) {
        //     flop ++;
        //     if (even_numbers[i]){
        //         sum ++;
        //         flop ++;
        //     }
        //         // world.log("processor %d, summed prime %d on index %d", pid, index_to_number(i, pid, p, flop), i);
        // }

        // put_numbers_to_all(sum, pid, p, local_primes);

        // world.sync();

        // world.log("==== Superstep 10");
        // for (auto partial_sum : local_primes) {
        //     sum += partial_sum;
        //     flop ++;
        // }

        // if (sum == (n - 2) / 2)
        //     world.log("yes");
        // else
        //     world.log("no");

        // flop += 3;

        flops[pid] = flop;
    });

    const auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    cout << "It took " << duration << " ms and " << flops[0] << " flops on processor 0" << endl;

    f.open("results.csv", ios_base::app);
    for (int i = 0; i < p; i ++) {
        f << n << ',' << p  << ',' << duration << ',' << i << ',' << flops.at(i) << ',' << 'p' << endl;
    }
    f.close();
}

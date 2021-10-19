#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>
#include "utils.hpp"

class Sieve {
    public:
        int pid, flop = 0;

        Sieve(size_t n, size_t p, int cyclic_local_size, bulk::coarray<bool>& primes, bulk::queue<int>& local_primes, bulk::world& world) : n(n), p(p), cyclic_local_size(cyclic_local_size), primes(primes), local_primes(local_primes), world(world) {
            pid = world.rank();
        }

        void parallel_sieve() {
            init_primes();
            sieve_local_multiples();
            sieve_remote_multiples();

        
            for (int i = 0; i < cyclic_local_size; i ++){
                if (primes[i])
                    world.log("number %d, prime? %d", pid + i * p + 1, primes[i]);
            }

        }
    private:
        size_t n, p;
        int cyclic_local_size;
        bulk::world& world;
        bulk::coarray<bool>& primes;
        bulk::queue<int>& local_primes;

        void init_primes(void) {
            for (int i = 0; i < cyclic_local_size; i ++){
                int current = index_to_number(i, pid, p, flop);

                flop ++;
                // world.log("\tprocessor %d, index %d, number %d", pid, i, current);
                if (current == 1)
                    primes[i] = false;
                else
                    primes[i] = true;
            }

            world.log("Sync-ing...");
            world.sync();
        }

        void sieve_local_multiples(void) {


            world.log("==== Superstep 2");
            // iterate through all the local numbers
            for (int i = 0; i < cyclic_local_size; i ++){
                flop ++;
                // world.log("\tprocessor %d, current %d", pid, current);

                // if the current number is marked as prime, communicate it to the other processors and sieve its multiples
                if (primes[i]){
                    int current = index_to_number(i, pid, p, flop);
                    flop ++;
                    put_prime_multiples(current, n, p, local_primes, flop, world);
                    // put_numbers_to_all(current, pid, p, local_primes);

                    // sieving will be done with a step-size of the current prime / gcd(current, p)
                    int d = gcd(current, p, flop);
                    int step = current / d;

                    flop += 2;

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
        }

        void sieve_remote_multiples(void) {
            world.log("==== Superstep 3");
            // for each of the received primes from the other processors, sieve any marked-as-prime local numbers
            for (auto prime : local_primes) {
                // world.log("\tprocessor %d received non-prime %d", pid, prime);

                flop ++;
                // start from the first multiple of the prime in the current boolean array
                for (int j = (prime - 1 - pid) / p; j < cyclic_local_size; j += prime) {
                    // int current = pid + j * p + 1;
                    // world.log("primes[%d] = %d", j, prime);
                    flop ++;
                    if (primes[j]) {
                        // world.log("\t\tprocessor %d, non-prime %d found", pid, prime);
                        primes[j] = false;
                    }
                }
            }

            world.log("Sync-ing...");
            world.sync();
        }

};
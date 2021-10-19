#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>
#include <math.h>

class Sieve {
    public:
        int pid, flop = 0;

        Sieve(size_t n, size_t p, int cyclic_local_size, bulk::coarray<bool>& primes, bulk::coarray<bool>& even_numbers, bulk::queue<int>& local_primes, bulk::queue<int>& local_sums, bulk::world& world) : n(n), p(p), cyclic_local_size(cyclic_local_size), primes(primes), local_primes(local_primes), local_sums(local_sums), even_numbers(even_numbers), world(world) {
            this->pid = this->world.rank();
            this->init_primes();
            this->init_even_numbers();
        }

        void parallel_sieve() {
            // init_primes();
            this->sieve_local_multiples();
            this->sieve_remote_multiples();

        
            for (int i = 0; i < this->cyclic_local_size; i ++){
                if (this->primes[i])
                    world.log("number %d, prime? %d", this->pid + i * this->p + 1, this->primes[i]);
            }

        }

        void twin_primes() {
            this->put_local_twins();
            this->get_remote_twins();
        }

        void goldbach_conjecture(void) {
            // Goldbach Conjecture
            // init_even_numbers();
            this->put_local_primes_to_all();
            this->sieve_local_sums();
            this->sieve_remote_sums();
            int n_even_numbers = this->count_even_numbers();

            if (n_even_numbers == (this->n - 2) / 2)
                world.log("yes");
            else
                world.log("no");

            this->flop += 3;
        }

    private:
        size_t n, p;
        int cyclic_local_size;
        bulk::world& world;
        bulk::coarray<bool>& primes;
        bulk::coarray<bool>& even_numbers;
        bulk::queue<int>& local_primes;
        bulk::queue<int>& local_sums;

        // calculate the number based on its index in a cyclically distributed boolean array
        int index_to_number(int i) {
            this->flop += 3;
            return this->pid + i * this->p + 1;
        }

        // calculate the index of the number in a cyclically distirbuted boolean array
        int number_to_index(int number) {       // return static_cast<int>(round(number / (p * 1.0)));
            return ceil((number - 1 - this->pid) / (this->p * 1.0));
        }

        // function that sends a number to all the other processors
        void put_number_to_all(int number) {
            for (int i = 0; i < this->p; i ++) {
                if (i != this->pid) { 
                    this->local_primes(i).send(number);
                }
            }
        }

        // function that sends the locally found primes to only the processors that might have multiples of it
        void put_prime_multiples(int prime) {
            this->flop ++;
            for (int i = prime * 2; i <= this->n; i += prime) {
                // world.log("send prime %d to processor %d", prime, (i - 1) % p);
                this->local_primes((i - 1) % this->p).send(i);
                this->flop += 3;
            }
        }
        void init_primes(void) {
            for (int i = 0; i < this->cyclic_local_size; i ++){
                int current = this->index_to_number(i);

                this->flop ++;
                // world.log("\tprocessor %d, index %d, number %d", pid, i, current);
                if (current == 1)
                    this->primes[i] = false;
                else
                    this->primes[i] = true;
            }

            world.log("Sync-ing...");
            this->world.sync();
        }

        void init_even_numbers(void) {
            // init even numbers array with 0 (not sieved);
            for (int i = 0; i < this->cyclic_local_size; i ++) {
                this->even_numbers[i] = false;
            }

            this->world.sync();
        }

        void put_local_primes_to_all(void) {
            world.log("==== Superstep 7");

            // auto local_sums = bulk::queue<int>(world);

            // send local primes to all other processors
            for (int i = 0; i < this->cyclic_local_size; i ++) {
                if (this->primes[i]){
                    int prime = this->index_to_number(i);
                        // send prime to all other processors
                    this->put_number_to_all(prime);
                }       
            }

            this->world.sync();
        }

        void sieve_local_multiples(void) {
            world.log("==== Superstep 2");
            // iterate through all the local numbers
            for (int i = 0; i < this->cyclic_local_size; i ++){
                this->flop ++;
                // world.log("\tprocessor %d, current %d", pid, current);

                // if the current number is marked as prime, communicate it to the other processors and sieve its multiples
                if (this->primes[i]){
                    int current = this->index_to_number(i);
                    this->flop ++;
                    this->put_prime_multiples(current);
                    // put_number_to_all(current, pid, p, local_primes);

                    // sieving will be done with a step-size of the current prime / gcd(current, p)
                    int d = this->gcd(current, this->p);
                    int step = current / d;

                    this->flop += 2;

                    for (int j = i + step; j < this->cyclic_local_size; j += step){
                        // int multiple = pid + j * p + 1;
                        // if (multiple % current == 0){
                            // world.log("\t\tprocessor %d, step %d, non-prime %d found", pid, step, pid + j * p + 1);
                            this->primes[j] = false;
                            this->flop ++;
                        // }
                    }
                }
                
            }

            world.log("Sync-ing...");
            this->world.sync();
        }

        void sieve_remote_multiples(void) {
            world.log("==== Superstep 3");
            // for each of the received primes from the other processors, sieve any marked-as-prime local numbers
            for (auto prime : this->local_primes) {
                // world.log("\tprocessor %d received non-prime %d", pid, prime);

                this->flop ++;
                // start from the first multiple of the prime in the current boolean array
                for (int j = (prime - 1 - this->pid) / this->p; j < this->cyclic_local_size; j += prime) {
                    // int current = pid + j * p + 1;
                    // world.log("primes[%d] = %d", j, prime);
                    this->flop ++;
                    if (this->primes[j]) {
                        // world.log("\t\tprocessor %d, non-prime %d found", pid, prime);
                        this->primes[j] = false;
                    }
                }
            }

            world.log("Sync-ing...");
            this->world.sync();
        }

        void put_local_twins(void) {
                // find Twin Primes
            world.log("==== Superstep 4");
            // send the local primes only to the processors that might possibly have its twin prime (which is a processor 2 units away from the current processor)
            for (int i = 0; i < this->cyclic_local_size; i ++) {
                this->flop ++;
                if (this->primes[i]) {
                    int current = this->index_to_number(i);
                    this->local_primes((this->pid + 2) % this->p).send(current);

                    this->flop += 2;
                    // world.log("\tprocessor %d sends %d to processor %d hoping for prime %d", pid, current, (pid + 2) % p, (current - 1 - (pid + 2) % p) / p + 2);
                }
            }
            world.log("Sync-ing...");
            this->world.sync();
        }

        void get_remote_twins(void) {
            world.log("==== Superstep 5");
            for (auto prime : this->local_primes) {
                this->flop += 2;
                int i = this->number_to_index(prime);
                // world.log("\tprocessor %d trying %d and %d on index %d as twin primes", pid, prime, prime, i);
                if (this->primes[i]) {
                    int current = this->index_to_number(i);
                    this->flop ++;
                    world.log("found twin primes (%d, %d)", prime, current);
                }
            }
        }

        void sieve_local_prime_sums(int prime1, int prime2) {
            int sum = prime1 + prime2;
            int sum_processor = (sum - 1) % p;
            // world.log("\tprocessor %d: prime1 = %d, prime2 = %d", pid, prime1, prime2);

            this->flop += 4;

            // if the prime sum is in the current processor, sieve it; otherwise send it to the corresponding processor
            if (sum <= this->n && sum % 2 == 0) {
                this->flop ++;
                if (sum_processor == this->pid) {
                    int k = this->number_to_index(sum);
                    // world.log("\tfound local summed prime %d on index %d", sum, k);
                    this->even_numbers[k] = true;
                }
                else {
                    // world.log("\tsend summed prime %d to processor %d", sum, sum_processor);
                    this->local_sums(sum_processor).send(sum);
                }

            }
        }

        void sieve_local_sums(void) {
            world.log("==== Superstep 8");
            // sieve local prime sums, and send remote sums to their corresponding processor
            for (int i = 0; i < this->cyclic_local_size; i ++) {
                this->flop ++;

                if (this->primes[i]) {
                    int prime1 = this->index_to_number(i);

                    for (int j = i; j < this->cyclic_local_size; j ++) {
                        this->flop ++;

                        if (this->primes[j]) {
                            // world.log("processor %d: i = %d, j = %d", pid,  i, j);
                            int prime2 = this->index_to_number(j);
                            // put_number_to_all(prime2, pid, p, local_primes);

                            this->sieve_local_prime_sums(prime1, prime2);
                        }
                    }

                    for (auto prime2 : this->local_primes) {
                        // world.log("processor %d got prime %d: i = %d", pid, prime2, i);
                        this->sieve_local_prime_sums(prime1, prime2);
                    }
                }
            }

            this->world.sync();
        }



        void sieve_remote_sums(void) {
            world.log("==== Superstep 8");
            // sieve the remotely gotten sums
            for (auto sum : this->local_sums) {
                int i = this->number_to_index(sum);
                // world.log("\tprocessor %d received summed prime %d on index %d", pid, sum, i);
                this->even_numbers[i] = true;
            }

            this->world.sync();
        }

        int count_even_numbers(void) {
            world.log("==== Superstep 9");

            // count how many even numbers larger than 2 are sums of primes
            int sum = 0;

            for (int i = 0; i < this->cyclic_local_size; i ++) {
                this->flop ++;
                if (even_numbers[i]){
                    sum ++;
                    this->flop ++;
                }
                    // world.log("processor %d, summed prime %d on index %d", pid, index_to_number(i), i);
            }

            this->put_number_to_all(sum);

            this->world.sync();

            world.log("==== Superstep 10");
            for (auto partial_sum : this->local_primes) {
                sum += partial_sum;
                this->flop ++;
            }

            return sum;
        }
    
        int gcd(int a, int b) {
            if (a == 0 || b == 0) {
                    this->flop += 2;
                    return 0;
            }
            else if (a == b) {
                    this->flop ++;
                    return a;
            }
            else if (a > b) {
                    this->flop += 2;
                    return gcd(a - b, b);
            }
            else {
                this->flop ++;
                return gcd(a, b - a);
            }
    }
};
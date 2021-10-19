// #include <bulk/backends/thread/thread.hpp>
// #include <bulk/bulk.hpp>
#include <string>
#include <chrono>
#include <fstream>
#include <vector>
#include "sieve.hpp"
// #include "utils.hpp"

using namespace std;


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
        auto cp = bulk::cyclic_partitioning<1>({n}, {p});
        auto cyclic_local_size = cp.local_count(world.rank());
        auto primes = bulk::coarray<bool>(world, cyclic_local_size);
        auto local_primes = bulk::queue<int>(world);
        
        Sieve sieve = Sieve(n, p, cyclic_local_size, primes, local_primes, world);
        sieve.parallel_sieve();

        flops[sieve.pid] = sieve.flop;
    });

    const auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    cout << "It took " << duration << " ms and " << flops[0] << " flops on processor 0" << endl;

    f.open("results.csv", ios_base::app);
    for (int i = 0; i < p; i ++) {
        f << n << ',' << p  << ',' << duration << ',' << i << ',' << flops.at(i) << endl;
    }
    f.close();
}

#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>
#include <string>
#include <chrono>
#include <fstream>
#include <vector>
#include "funcs.hpp"
#include "seq_sieve_optimal.cpp"
#include "twins.hpp"

using namespace std;


int main(int argc, char* argv[]) {
    bulk::thread::environment env;
    size_t n, p = env.available_processors();
    size_t b = 2 * (p + 1);
    ofstream f;
    vector<size_t> flops = vector<size_t>(p);

    for (int i = 1; i < argc; i ++) {
        string arg = argv[i];
        if (arg == "-n") {
            n = static_cast<size_t>(stoi(argv[++ i]));
        } else if (arg == "-p") {
            p = static_cast<size_t>(stoi(argv[++ i]));
        } else if (arg == "-b") {
            b = static_cast<size_t>(stoi(argv[++ i]));
        } else {
            cerr << "wrong arguments";
        }
    }

    const auto start = chrono::steady_clock::now();

    env.spawn(p, [&n, &p, &b, &flops](auto& world) {
        // init local processors
        auto pid = world.rank(); // local processor ID
        size_t flop = 0;

        auto pair = distribute_small_numbers(b, pid, p, flop);
        // auto local_s = pair.first;
        auto s_winners = pair.second;
        auto twin_dist = twin_distribute_s(p, pid, b, s_winners, world);
        auto local_s = twin_dist.second;
        auto local_pairs = twin_dist.first;

        for (auto s : local_s){
            world.log("%d, %d", s, pid);}

        map<size_t, map<size_t, size_t>> inverse_s;
        map<size_t, vector<size_t>> local_prime_bools;
        map<size_t, vector<size_t>> local_primes;
        auto shared = bulk::queue<size_t, size_t[]>(world);

        for (auto s : local_s) {
            inverse_s.insert({s, inverses(b, s, s_winners, flop)});
        }

        for (auto s : local_s) {
            auto pair = local_sieve(s, b, n, flop);
            local_prime_bools.insert({s, pair.first});
            local_primes.insert({s, pair.second});

            for (size_t i = 0; i < p; i ++) {
                shared(i).send(s, local_primes[s]);
            }
        }

        world.sync();

        for (auto [remote_s, remote_primes] : shared) {
            for (auto s : local_s) {
                for (auto a : remote_primes) {
                    sieve_all_multiples(s, b, a, remote_s, local_prime_bools[s], inverse_s[s], flop);
                }
            }
        }

        vector <std::pair < size_t, size_t > > all_local_twin_primes;
        for (auto p : local_pairs) {
            size_t s = p.first;
            size_t t = p.second;
            if (s==1) {
                for (size_t k = 1; k < local_prime_bools[s].size(); k ++) {
                    if (local_prime_bools[s].at(k)) {
                        if (local_prime_bools[t].at(k - 1)) {
                            if (t + k * b <= n){
                                all_local_twin_primes.push_back(make_pair(s + k * b, t + (k - 1) * b));
                                world.log("%d, %d, %d, %d, %d", s+k*b, t+(k-1)*b ,s,t, pid);
                            }
                        }
                    }
                }
            }
            else { 
                for (size_t k = 0; k < local_prime_bools[s].size(); k ++) {
                    if (local_prime_bools[s].at(k)) {
                        if (local_prime_bools[t].at(k)) {
                            if (t + k * b <= n) {
                                all_local_twin_primes.push_back(make_pair(s + k * b, t + k * b));
                                world.log("%d, %d, %d, %d, %d", s+k*b, t+k*b ,s,t, pid);
                            }
                        }
                    }
                }
            }
        }
        
        flops[pid] = flop;
    });

    const auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "It took " << duration << " ms and " << flops[0] << " flops on processor 0" << endl;

    f.open("results.csv", ios_base::app);
    for (int i = 0; i < p; i ++) {
        f << n << ',' << p  << ',' << duration << ',' << i << ',' << flops.at(i) << ',' << 'o' << endl;
    }
    f.close();
}

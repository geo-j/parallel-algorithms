#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>
#include <string>
#include <chrono>
#include <fstream>
#include <vector>
#include "funcs.hpp"
#include "seq_sieve_2.cpp"

using namespace std;


int main(int argc, char* argv[]) {
    bulk::thread::environment env;
    size_t n, p = env.available_processors();
    int b = 2 * (p + 1);
    ofstream f;
    vector<int> flops = vector<int>(p);

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
        int flop = 0;

        auto pair = b_coprimes(b, pid, p, world);
        auto my_s = pair.first;
        auto s_winners = pair.second;

        map<int, map<int, int>> my_inverses;
        map<int, vector<int>> localarray;
        map<int, vector<int>> localprimelist;
        auto shared = bulk::queue<int, int[]>(world);

        for (auto s : my_s) {
            my_inverses.insert({s, inverse_dict(b, s_winners, s, world)});
        }

        for (auto s : my_s) {
            auto pair = localSieve(s, b, n, world);
            localarray.insert({s, pair.first});
            localprimelist.insert({s, pair.second});

            for (int i = 0; i < p; i ++) {
                shared(i).send(s, localprimelist[s]);
            }
        }

        world.sync();

        for (auto [remote_s, remoteprimelist] : shared) {
            for (auto s : my_s) {
                for (auto a : remoteprimelist) {
                    remove_multiples(s, b, localarray[s], a, remote_s, my_inverses[s], world);
                }
            }
        }

        for (auto s : my_s) {
            for (int k = 0; k < localarray[s].size(); k ++) {
                if (localarray[s].at(k)) {
                    if (s + k * b < n){
                        world.log("%d", s + k * b);
                        }
                }
            }
        }

        if (pid == p - 1) {
            vector<int> primestob = primesupto(b, world);

            for (auto x : primestob)
                world.log("%d", x);
        }
        flops[pid] = flop;
    });

    const auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    // cout << "It took " << duration << " ms and " << flops[0] << " flops on processor 0" << endl;

    f.open("results.csv", ios_base::app);
    for (int i = 0; i < p; i ++) {
        f << n << ',' << p  << ',' << duration << ',' << i << ',' << flops.at(i) << endl;
    }
    f.close();
}

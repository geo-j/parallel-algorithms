#include <chrono>
#include <fstream>
// #include "funcs.hpp"
#include <numeric>
#include <vector>
#include <map>
#include <algorithm>
#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>

using namespace std;

const int SYNC_TIME = 100;

struct work {
    long long int w;
    vector<int> visited;
    long long int count;
    long long int N;
};

void saw(bulk::world &world, long long int n, long long int N, long long int v, vector<vector<int>> A, long long int i, vector<int> visited, vector<long long int> walk, long long int &count, long long int p, long long int pid, vector<work> &work_stack, vector<long long int> &final_nodes, long long int cur_N) {
    /*
    Input: the world, the time since last sync
    the int n, representing the size of graph
    the int N, representing the length of the paths we are interested in
    the int v, representing the node we are at
    the matrix A, which is the adjecency matrix of our graph
    the path length so far 
    the vector of all nodes we have visited so far vector<int> visited, 
    the vector<long int> walk, 
    long int &count, how many walks has this processor counted. , 
    long int p, 
    long int pid

    a boolean top which indicates whether this process is the topmost called by the current processor. 
    */

    world.log("processor %d finding all paths of length %d starting at length %d and node %d", pid, cur_N, i, v);
    world.log("trying node %d, visited? %d", v, visited.at(v));
    if (!visited.at(v)) {
        // world.log("i = %d, cur_N = %d, N = %d", i, cur_N, N);
         if (i == cur_N) {
            // for (long int j = 0; j < walk.size(); j ++) {
            //     world.log("%d, ", walk.at(j));
            // }
            count ++;

            world.log("processor %d finished at starting node = %d, count = %d, path length = %d", pid, v, count, i);
            for (long long int i = 0; i < visited.size(); i ++) {
                    if (visited.at(i))
                        world.log("%d,", i);
                }
            world.log("%d", v);

            if (i < N) {
                work_stack.push_back(work(v, vector<int>(visited), count, cur_N + 1));
                // final_nodes.push_back(v);
                // work.visited = vector<int>(visited);
            }
                
        }
        else {
            world.log("\tvisiting node %d", v);
            visited[v] = true;
            for (long int w = 0; w < n; w ++) {
                if (A[v][w]) {
                    // work_stack.push_back(work(w, vector<int>(visited), count, cur_N ));
                    walk.push_back(w);
                    saw(world, n, N, w, A, i + 1, visited, walk, count, p, pid, work_stack, final_nodes, cur_N);
                    walk.pop_back();
                }
            }
            visited[v] = false;
            world.log("\tunvisiting node %d", v);

        }
    }
}

void send_dones(bulk::world &world, long long int p, long long int pid, int done, bulk::queue<int> &send_done) {
    // auto send_done = bulk::queue<int>(world);

    for (long long int i = 0; i < p; i ++) {
            send_done(i).send(done);
    }
}

void send_works(bulk::world &world, long long int p, long long int pid, vector<work> work_stack, vector<long long int> final_nodes, bulk::queue<long long int, int[], long long int, long long int> &send_work) {
    if (!work_stack.empty())
        for (long long int i = 0; i < work_stack.at(0).visited.size(); i ++) {
            world.log("node %d is visited? %d", i, work_stack.at(0).visited.at(i));
        }
    // auto send_work = bulk::queue<long long int, int[], long long int, long long int>(world);

    for (long long int i = 0; i < p; i ++) {
        if (i != pid) {
            for (auto work : work_stack) {
                // for (auto node : final_nodes) {
                    send_work(i).send(work.w, work.visited, work.count, work.N);
                    world.log("processor %d sends starting node = %d, count = %d, path length = %d", pid, work.w, work.count, work.N);
                // }
            }
        }
    }
}

map<long long int, int> redistribute_work(bulk::world &world, vector <int> work_stack_lengths, long long int pid, long long int p) {
    /*
        Input ::
        world, same as always
        work_stack_lengths: for every processor we have the length of their work stack
        pid:: which processor are we
        p:: how many processors are there

        Output::
        A map from int to int, representing how much work this processor should send to each processor (so if the map is [(1,100), (4,20)] we'll send 100 work to processor 1 and 20 work to processor 4)
    */

    long long int total_workload = accumulate(work_stack_lengths.begin(), work_stack_lengths.end(), 0);
    // long long int ideal_fair_workload = static_cast<long long int>(total_workload / p);
    //The best workload is sorta cyclically distributed
    vector<long long int> fair_work_stack_lengths(p, 0);
    
    // Our ideal workload is one where each processor has about the same amount of work, 
    // This distribution is inspired by a cyclic work distribution, which we use to not deal with rounding errors. 
    long long int current_pid = 0;
    for (int i = 0; i < total_workload; i ++) {
        fair_work_stack_lengths[current_pid] ++;
        current_pid = (current_pid + 1) % p; 
    } 

    map<long long int, int> sharing_map;
    //the following loop will go trough how the work should be offloaded by everyone 
    //when we are at our own processor and we see we should offload work, 
    // we'll keep track of it and actually put it in the map
        // for (long long int current_offloader = 0; current_offloader < pid + 1; current_offloader ++) {
    for (long long int current_offloader = 0; current_offloader < pid + 1; current_offloader ++) {
        long long int current_receiver = 0;
        while (work_stack_lengths[current_offloader] - fair_work_stack_lengths[current_offloader] > 0) {    // as long as we still have work to offload
            long long int receiver_needs = work_stack_lengths[current_receiver] - fair_work_stack_lengths[current_receiver];
            if (receiver_needs < 0) {    // for anyone who still needs work
                long long int can_offload = work_stack_lengths[current_offloader] - fair_work_stack_lengths[current_offloader];
                long long int offloaded;
                if (abs(receiver_needs) > can_offload) {
                    work_stack_lengths[current_offloader] -= can_offload;
                    work_stack_lengths[current_receiver]  += can_offload;
                    offloaded = can_offload;
                }
                else {
                    //keep in mind that receiver needs is negative
                    work_stack_lengths[current_offloader] += receiver_needs;
                    work_stack_lengths[current_receiver]  -= receiver_needs;
                    offloaded = -receiver_needs;

                }
                if (current_offloader == pid) {
                    sharing_map.insert(std::pair(current_receiver, offloaded));
                }

                // work_stack_lengths[current_offloader] -= receiver_needs;
                // work_stack_lengths[current_receiver]  += receiver_needs;
                // // Now the receiver should have all the work it needs, but we might have overshot a bit, in which case we need to go back 
                // long long int overshoot = work_stack_lengths[current_offloader] - fair_work_stack_lengths[current_offloader]; 
                // if (overshoot < 0) {
                //     work_stack_lengths[current_offloader] += overshoot;
                //     work_stack_lengths[current_receiver]  -= overshoot;
            }
            current_receiver ++;
        }
    }

    return sharing_map;
}
    



int main(int argc, char* argv[]) {
    bulk::thread::environment env;
    long int n, N, v, p = env.available_processors();
    ofstream f_out;
    vector<long int> flops(p);
    vector<vector<int>> A;

    for (int i = 1; i < argc; i ++) {
        string arg = argv[i];
        if (arg == "-p") {
            p = static_cast<long long int>(stoi(argv[++ i]));
        } else {
            cerr << "wrong arguments";
        }
    }

    cin >> n;
    ifstream f_in("input_" + to_string(n));
    for (long long int i = 0; i < n; i ++) {
        vector<int> row(n);
        A.push_back(row);
        for (long int j = 0; j < n; j ++) {
            int edge;
            f_in >> edge;
            A[i][j] = edge;
        }
    }
    cin >> N >> v;

    const auto start = chrono::steady_clock::now();

    env.spawn(p, [&n, &p, &flops, &N, &v, &A](auto& world) {
        // init local processors
        auto pid = world.rank(); // local processor ID
        long long int flop = 0;
        long long int count = 0;
        int time_since_last_sync = SYNC_TIME - 2;
        long long int cur_N = 1;
        vector<work> work_stack;
        vector<int> visited(n, false);
        vector<long long int> walk;
        walk.push_back(v);
        work_stack.push_back(work(v, visited, count, cur_N));
        auto send_work = bulk::queue<long long int, int[], long long int, long long int>(world);
        auto send_done = bulk::queue<int>(world);


        int done = false;
        while (!done) {
        // for (int i = 0; i < 2; i ++) {
            for (auto [v, visited, count, cur_N] : send_work) {
                work_stack.push_back(work(v, visited, count, cur_N));
                world.log("processor %d received starting node = %d, count = %d, cur_N = %d", pid, v, count, cur_N);
            }

            if (work_stack.empty()) {
                done = true;
            } else {
                work work = work_stack.at(work_stack.size() - 1);
                work_stack.pop_back();
                vector<long long int> final_nodes;
                world.log("processor %d does work starting node = %d, count = %d, path length = %d", pid, work.w, work.count, work.N);


                saw(world, n, N, work.w, A, work.N - 1, work.visited, walk, work.count, p, pid, work_stack, final_nodes, work.N);

                if (work.N < N) {
                    // for (auto node : final_nodes) {
                    //     work_stack.push_back({node, work.visited, work.count, cur_N + 1});
                    // }
                    world.log("still got work to do, cur_N = %d, N = %d, found %d paths so far", work.N, N, work.count);
                    send_works(world, p, pid, work_stack, final_nodes, send_work);
                }
            }
            // get work

            send_dones(world, p, pid, done, send_done);
            world.sync();

            for (auto remote_done : send_done) {
                if (!remote_done) {
                    done = false;
                }
            }
        }

        flops[pid] = flop;

    });

    const auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    cout << "It took " << duration << " ms and " << flops[0] << " flops on processor 0" << endl;

    f_out.open("results.csv", ios_base::app);
    for (int i = 0; i < p; i ++) {
        f_out << n << ',' << p  << ',' << duration << ',' << i << ',' << flops.at(i) << ',' << 'p' << endl;
    }
    f_out.close();
}

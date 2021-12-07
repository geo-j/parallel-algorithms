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

struct work {
    long long int v;
    vector<int> visited;
    // long long int count;
    long long int cur_path_length;
};

// OPTIMISATIONS:
// TODO: use an adjacency list instead of a matrix
// TODO: make visited an integer instead of a  bool array
// TODO: make redistribute work cyclically but not by going trough everything (do clever divisions)

// void saw(bulk::world &world, long long int n, long long int N, long long int v, vector<vector<int>> A, long long int i, vector<int> visited, vector<long long int> walk, long long int &count, long long int p, long long int pid, vector<work> &work_stack, long long int cur_N) {
void saw(bulk::world &world, long long int n, long long int N, long long int v, vector<vector<int>> A, long long int i, vector<int> visited, long long int &count, long long int p, long long int pid, vector<work> &work_stack) {
    
    /*
    Input: the world, the time since last sync
    the int n, representing the size of graph
    the int N, representing the length of the paths we are interested in
    the int v, representing the node we are at
    the int i, representing the current path length // change to cur_path_length
    the matrix A, which is the adjecency matrix of our graph
    the vector of all nodes we have visited so far vector<int> visited,  
    long int &count, how many walks has this processor counted. , 
    long int p, 
    long int pid

    Effects:
        first checks whether we have visited the vector, if not we don't do anything. 
            if the path length is the maximal length we increment the count
            otherwise, we add all neighbours of v to the workstack as if we were travelling there. 
    */

    // world.log("processor %d finding all paths of length %d starting at length %d and node %d", pid, cur_N, i, v);
    // world.log("trying node %d, visited? %d", v, visited.at(v));
    if (!visited.at(v)) {
        // world.log("SAW called from processor %d at node %d",  pid, v);
            /*
            //First we check whether we are done
            if (cur_path_length == N ){
                count ++ ;
            }
            else {
                visited [v] = true 
                for  w neighbours of v
                add ( w, cur_paht_length +1, visited) to workstack 
            }
            */

        // world.log("i = %d, cur_N = %d, N = %d", i, cur_N, N);
         if (i == N) {
            // for (long int j = 0; j < walk.size(); j ++) {
            //     world.log("%d, ", walk.at(j));
            // }
            count ++;
            world.log("processor %d finished at ending node = %d, count = %d", pid, v, count);
            // for (long long int i = 0; i < visited.size(); i ++) {
            //         if (visited.at(i))
            //             world.log("%d,", i);
            //     }
            // world.log("%d", v);

            // if (i < N) {
            //     work_stack.push_back(work(v, vector<int>(visited), count, cur_N + 1));
            //     // final_nodes.push_back(v);
            //     // work.visited = vector<int>(visited);
            // }
                
        }
        else {
            // world.log("\tvisiting node %d", v);
            visited[v] = true;
            for (long int w = 0; w < n; w ++) {
                if (A[v][w]) {
                    // We should take care to send a copy of visited as it will be different the next time we call it. 
                    work_stack.push_back(work(w, vector<int>(visited), i + 1));
                    // walk.push_back(w);
                    // saw(world, n, N, w, A, i + 1, visited, walk, count, p, pid, work_stack, cur_N);
                    // walk.pop_back();
                }
            }
            // visited[v] = false;
            // world.log("\tunvisiting node %d", v);

        }
    }
}

// void send_dones(bulk::world &world, long long int p, long long int pid, int done, bulk::queue<int> &send_done) {
//     // auto send_done = bulk::queue<int>(world);

//     for (long long int i = 0; i < p; i ++) {
//             send_done(i).send(done);
//     }
// }

void send_work_stack_lengths(bulk::world &world, long long int p, long long int pid, int work_stack_length, bulk::queue<long long int, int> &send_work_stack_length) {
    for (long long int i = 0; i < p; i ++) {
        send_work_stack_length(i).send(pid, work_stack_length);
    }
}

void send_works(bulk::world &world, long long int p, long long int pid, vector<work> &work_stack, bulk::queue<long long int, int[], long long int> &send_work, map<long long int, int> shared_works) {
    // if (!work_stack.empty())
    //     for (long long int i = 0; i < work_stack.at(0).visited.size(); i ++) {
    //         world.log("node %d is visited? %d", i, work_stack.at(0).visited.at(i));
    //     }
    // auto send_work = bulk::queue<long long int, int[], long long int, long long int>(world);

    // for (long long int i = 0; i < p; i ++) {
    //     if (i != pid) {
    //         for (auto work : work_stack) {
    //             // for (auto node : final_nodes) {
    //                 send_work(i).send(work.w, work.visited, work.count, work.N);
    //                 world.log("processor %d sends starting node = %d, count = %d, path length = %d", pid, work.w, work.count, work.N);
    //             // }
    //         }
    //     }
    // }
    for (std::pair<long long int, int> shared_work : shared_works){
        long long int target_processor = shared_work.first;
        long long int offloaded_workload = shared_work.second;
        for (long long int i = 0; i < offloaded_workload; i ++) {
            work work_to_send = work_stack.at(work_stack.size() - 1);
            work_stack.pop_back();
            send_work(target_processor).send(work_to_send.v, work_to_send.visited, work_to_send.cur_path_length);
        }
    }
                
}

map<long long int, int> redistribute_work(bulk::world &world, vector <long long int> work_stack_lengths, long long int pid, long long int p) {
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
        long long int flop       = 0;
        long long int count      = 0;
        long long int SYNC_TIME  = 0;
        long long int time_since_last_sync = 0;
        long long int path_length_so_far = 0;
        vector<work> work_stack;
        vector<int> visited(n, false);
        vector<long long int> final_nodes;
        vector<long long int> walk;
        walk.push_back(v);

        work_stack.push_back(work(v, visited, path_length_so_far));
        
        auto send_work              = bulk::queue<long long int, int[], long long int>(world);
        // auto send_done              = bulk::queue<int>(world);final_nodes
        auto send_work_stack_length = bulk::queue<long long int, int>(world);
        // auto send_work              = bulk::queue<int>(world);


        int done = false;
        while (!done) {
        // for (int i = 0; i < 2; i ++) {
            // for (auto [v, visited, count, cur_N] : send_work) {
            //     work_stack.push_back(work(v, visited, count, cur_N));
            //     world.log("processor %d received starting node = %d, count = %d, cur_N = %d", pid, v, count, cur_N);
            // }

            if (work_stack.empty()) {
                done = true;
            } 
            else {
                work work = work_stack.at(work_stack.size() - 1);
                work_stack.pop_back();
                // world.log("processor %d does work starting node = %d, count = %d, path length = %d", pid, work.w, count, work.N);


                saw(world, n, N, work.v, A, work.cur_path_length, work.visited, count, p, pid, work_stack);

                // if (work.N < N) {
                //     // for (auto node : final_nodes) {
                //     //     work_stack.push_back({node, work.visited, work.count, cur_N + 1});
                //     // }
                //     world.log("still got work to do, cur_N = %d, N = %d, found %d paths so far", work.N, N, work.count);
                //     send_works(world, p, pid, work_stack, send_work);
                // }
            }

            
            // get work
            // In order to redistribute the work, we share the lengths of our work stacks with everybody, and unpack it later. 
            //Syncing method below. 
            if (time_since_last_sync == SYNC_TIME) {
                //First we share how much work we have 
                send_work_stack_lengths(world, p, pid, work_stack.size(), send_work_stack_length);
                // send_dones(world, p, pid, done, send_done);
                world.sync();
                //Then we receive the work.  

                vector<long long int> work_stack_lengths(p, 0);
                for (auto [remote_pid, remote_work_stack_length] : send_work_stack_length) {
                    work_stack_lengths[remote_pid]  = remote_work_stack_length;
                }
                // We check if we are done
                done = true;
                for (long long int i = 0 ; i < work_stack_lengths.size(); i ++){
                    if (work_stack_lengths[i] != 0 ){
                        done  = false;
                    }
                }

                //Now, we calculate how the work should be redistributed.
                map<long long int, int> shared_work = redistribute_work(world, work_stack_lengths, pid, p);
                // Then, we share the work we need to share
                send_works(world, p, pid, work_stack, send_work, shared_work);
                world.sync();
                //Finally we add the work we received  to our workstack 
                 
                for (auto [v, visited, cur_N] : send_work) {
                    work_stack.push_back(work(v, visited, cur_N));
                    SYNC_TIME = work_stack.size();
                    time_since_last_sync = 0; 
                    // world.log("processor %d received starting node = %d, count = %d, cur_N = %d", pid, v, count, cur_N);
                }
                //we now should have workstacks of comparable size for each processor.
            }



            
                //Then we work out wether we're done and if not, how we should redistribute the work
                
                //Then we redistribute the work.

                //we should start working on the stack again, but first we are going to reset our time_since_last_sync and our  


                /* below is notepad
                While !done:
                    For (i = 0 ; i<synctime ; i++ ){
                        saw one step of the workstack 
                    }

                    Share workstack lengths
                    sync
                    find done's 
                    Redistribute work
                    sync
                    We know have a new workstack. 
                    synctime = workstack.size()

                */

            }

            // for (auto remote_done : send_done) {
            //     if (!remote_done) {
            //         done = false;
            //     }
            // }


        // }

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

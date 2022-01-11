#include <chrono>
#include <fstream>
#include <numeric>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <bitset>
#include <string>
#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>

using namespace std;

/* This code counts the number of self-avoiding walks on a given graph with a given starting point
 * It does so in a parallel fashion by load-balancing workstacks of different processors. 
 */

struct work {
/* A piece of work consists of a path,
 * we keep track of -the end point of our path
 * 		    -the nodes we have visited in our path in the form of a boolean vector
 * 		    -the length of the path. 
 */
    long long int v;
    vector<int> visited;
    long long int cur_path_length;
};

// OPTIMISATIONS:
// TODO: make visited an integer instead of a  bool array
// NOTWORTHIT: Make worstack a stack instead of a vector?
// TODO: Make a neighbours function which can use a graph from suitesparse/ 
// DONE: generate a square lattice by calculations so we save memory)
// DONE: use an adjacency list instead of a matrix
// DONE: only pass the workstack to saw
// DONE? make redistribute work cyclically but not by going trough everything (do clever divisions)
// TODO: Implement it for the Rubik's Snake
// TODO: on Rob's advice: make visited int list instead of bool array. 
// TODO: overdecomposition

vector<long long int> neighbours(long long int N, int d, long long int v, long long int pid, vector<long long int> &flops){
    /* Input:
        d for the dimension
        N for the max size of the walk
        v for the node we need the d-dimensional neighbours of

        Output:
        a list of all neighbours of v in a d-dimensional grid of size N
        the centre of this grid should have coordinate pow(n+2,d). 
        Then in a diamond of size N around this centre, 
        the neigbhours should match up, and all have positive values. 
    */
    vector<long long int> neighbors;

    for (int i = 0 ; i < d; i ++){
        neighbors.push_back(v + pow((N + 2), i));
        neighbors.push_back(v - pow((N + 2), i));

        flops[pid] += 7;
    }

    return neighbors;
}

vector<work> starting_L (long long int N, int d, long long int pid, vector<long long int> &flops){
    /* Should give all paths going left i times and then upwards, in the counting as in the neighbour function.

    Input 
        N: maximal path length
        d: dimensions. 

    In a regular square lattice we can make use of symmetry to do less work. 
    We can assume all starting paths are of an L-shape: first we go in one direction i times, then we take one step in another direction. 
    Output: all starting work of such paths. 

    How often should we count such an L-shaped path?
     well we can go in 2d directions for the first step
     and the direction of the "short bit of the L" can go in d-1 directions, and then both in the positive and negative direction 
     so totally, we should count these 4d(d-1) times.    
    
    There is also a path which is not L-shaped, but | -shaped, a straight line. This also happens 2d times, 
    Together the |-shape of size N and all the L-shaped give all starting paths modulo symmetry. 

    
    We can distribute the L-shaped starting paths cyclically,
    multiply this end result by 4d(d-1) and add 2d. 

    add 2d tot the count to account for the | -shaped path

      
    
    */
    vector<work> starting_positions;
    
    long long int centre = pow(N + 2, d) ;
    long long int startpos = centre + N + 2; 
    vector<int> visited;
    // auto visited= vector<int>(2 * centre);
    // visited[centre] = 1;
    visited.push_back(centre);

    flops[pid] += 5;

    for (int i = 1; i < N; i ++) {
        // visited[centre + i] = 1;
        visited.push_back(centre + i);
        startpos ++; 
        work workload =  {startpos, vector<int>(visited), i + 1};
        starting_positions.push_back(work(workload));
        flops[pid] += 4;
    }
    return vector<work>(starting_positions);
}

void saw(int d, long long int N, long long int &count, long long int p, long long int pid, vector<work> &work_stack, vector<long long int> &flops) {
/* given a path adds all of its one-step extensions to the workstack
 *
 * Input: the world, the time since last sync
 *        the int n, representing the size of graph
 *        the int N, representing the length of the paths we are interested in
 *        the int v, representing the node we are at
 *        the int i, representing the current path length // change to cur_path_length
 *        the matrix A, which is the adjecency matrix of our graph
 *        the vector of all nodes we have visited so far vector<int> visited,  
 *        long int &count, how many walks has this processor counted. , 
 *        long int p, 
 *        long int pid
 *
 * Effects:
 *    first checks whether the given path is still self-avoiding. 
 *        if the path length is the needed one we increment the count
 *        otherwise, we add all neighbours of v to the workstack as if we were travelling there. 
 */
    work task = work_stack.at(work_stack.size() - 1);
    work_stack.pop_back();
    //First we check whether we have visited this node already 
    // if (!task.visited.at(task.v)) {
    auto it = find(task.visited.begin(), task.visited.end(), task.v);
    if (it == task.visited.end()) {
	 // Then we check whether our path is of the needed length
         if (task.cur_path_length == N) {
            count ++;
        }
	// If not, we will add all extensions of the path to our workstack
        else {
            // task.visited[task.v] = true;
            task.visited.push_back(task.v);

            for (auto w : neighbours(N, d, task.v, pid, flops))
                    // We should take care to send a copy of visited as it will be different the next time we call it. 
                    work_stack.push_back(work(w, vector<int>(task.visited), task.cur_path_length + 1));
                // }
            // }
        }
    }

    flops[pid] += 3;
}

void send_work_stack_lengths(long long int p, long long int pid, int work_stack_length, bulk::queue<long long int, int> &send_work_stack_length, vector<long long int> &flops) {
/* shares the lengths of the work stacks to all other processors
 */	
    for (long long int i = 0; i < p; i ++) {
        send_work_stack_length(i).send(pid, work_stack_length);

        flops[pid] ++;
    }
}

void send_works(long long int p, long long int pid, vector<work> &work_stack, bulk::queue<long long int, int[], long long int> &send_work, map<long long int, int> shared_works, vector<long long int> &flops) {
/* sends the work according to the redistribution method as below */
    for (std::pair<long long int, int> shared_work : shared_works){
        long long int target_processor = shared_work.first;
        long long int offloaded_workload = shared_work.second;

        for (long long int i = 0; i < offloaded_workload; i ++) {
            work work_to_send = work_stack.at(work_stack.size() - 1);
            work_stack.pop_back();
            send_work(target_processor).send(work_to_send.v, work_to_send.visited, work_to_send.cur_path_length);

            flops[pid] += 2;
        }
    }
                
}

map<long long int, int> redistribute_work(vector <long long int> work_stack_lengths, long long int pid, long long int p, vector<long long int> &flops) {
/*
 * Input ::
 * world, same as always
 * work_stack_lengths: for every processor we have the length of their work stack
 * pid:: which processor are we
 * p:: how many processors are there

 * Output::
 * A map from int to int, representing how much work this processor should send to each processor 
 * Example :if the map is [(1,100), (4,20)] we'll send 100 work to processor 1 and 20 work to processor 4
 */
    //First calculate the total amount of work
    long long int total_workload = accumulate(work_stack_lengths.begin(), work_stack_lengths.end(), 0);

    flops[pid] += work_stack_lengths.size();

    // Our ideal workload is one where each processor has about the same amount of work, 
    vector<long long int> fair_work_stack_lengths(p, 0);
	long long int fair_workload        = total_workload / p;
	long long int procs_with_more_work = total_workload % p;

    flops[pid] += 2;
	
	for (long long int i = 0; i < p; i ++){
        fair_work_stack_lengths[i] = fair_workload;
        if (i < procs_with_more_work){
            fair_work_stack_lengths[i] ++;

            flops[pid] ++;
        }
        flops[pid] ++;
	}

    // for (long long int i = 0; i < p; i ++)
    //     loads[i].push_back(fair_work_stack_lengths.at(i));
//    // Our ideal workload is one where each processor has about the same amount of work, 
//    // This distribution is inspired by a cyclic work distribution, which we use to not deal with rounding errors. 
//    long long int current_pid = 0;
//    for (int i = 0; i < total_workload; i ++) {
//        fair_work_stack_lengths[current_pid] ++;
//        current_pid = (current_pid + 1) % p; 
//    } 

    map<long long int, int> sharing_map;
    //the following loop will go trough how the work should be offloaded by everyone 
    //when we are at our own processor and we see we should offload work, 
    // we'll keep track of it and actually put it in the map
    flops[pid] ++;
    for (long long int current_offloader = 0; current_offloader < pid + 1; current_offloader ++) {
        long long int current_receiver = 0;
        while (work_stack_lengths[current_offloader] - fair_work_stack_lengths[current_offloader] > 0) {
			// as long as we still have work to offload we give work to anyone who needs it
            long long int receiver_needs = work_stack_lengths[current_receiver] - fair_work_stack_lengths[current_receiver];

            flops[pid] += 2;  
            if (receiver_needs < 0) {    // for anyone who still needs work
                long long int can_offload = work_stack_lengths[current_offloader] - fair_work_stack_lengths[current_offloader];
                long long int offloaded;
				//We should not give away so much work that we don' t have any anymore
                flops[pid] ++;
                if (abs(receiver_needs) > can_offload) {
                    work_stack_lengths[current_offloader] -= can_offload;
                    work_stack_lengths[current_receiver]  += can_offload;
                    offloaded = can_offload;

                    flops[pid] += 2;
                }
                else {
                    //keep in mind that receiver needs is negative
                    work_stack_lengths[current_offloader] += receiver_needs;
                    work_stack_lengths[current_receiver]  -= receiver_needs;
                    offloaded = -receiver_needs;

                    flops[pid] += 3;
                }
				// Now if we have given away work, we should keep track of it so we can actually do it later
                if (current_offloader == pid) {
                    sharing_map.insert(std::pair(current_receiver, offloaded));
                }
            }
            current_receiver ++;

            flops[pid] += 2;
        }
    }
    return sharing_map;
}
    



int main(int argc, char* argv[]) {
	//Starting the bulk system and taking user input
    bulk::thread::environment env;
    long long int N, v, p = env.available_processors(), n_paths = 2; // get available processors
    ofstream f_out;
    vector<vector<int>> A;
    int d;

    
    for (int i = 1; i < argc; i ++) {
        string arg = argv[i];
        if (arg == "-p") {  // use a flag for the number of processors. if none, then the maximum is chosen
            p = static_cast<long long int>(stoi(argv[++ i]));
        } else {
            cerr << "wrong arguments";
        }
    }

    vector<long long int> flops(p), syncs(p);

    cin >> d;

    cin >> N;
    v = pow(N + 2, d);
	// cout << "N = " << N <<", d = " << d << ", v = " << v << ", p = " << p << endl; 

    const auto start = chrono::steady_clock::now();

    if (d >= 2) {
        env.spawn(p, [&d, &p, &flops, &N, &v, &syncs, &n_paths](auto& world) {
            // init local processors
            auto pid = world.rank(); // local processor ID
            long long int flop       = 0;
            long long int count      = 0;
            long long int SYNC_TIME  = 0;
            long long int time_since_last_sync = 0;
            long long int path_length_so_far = 0;
            vector<work> work_stack;
            vector<int> visited;
            auto start_sync = chrono::steady_clock::now();
            auto begin = chrono::steady_clock::now();




        //Initialize processor 0 as the first one with work, no on else has any yet
            // if (pid == 0){
            //     work_stack.push_back(work(v, visited, path_length_so_far));
            // }

        
        //distribute  the starting positions cyclically
            auto starting_positions = starting_L(N, d, pid, flops);
            // world.log("There are %d starting positions",starting_positions.size());
            // world.sync();
            // world.log("I see a path length of %d", starting_positions.at(0).cur_path_length);
            for (long long int i = pid; i < starting_positions.size(); i += p){
                work_stack.push_back(starting_positions.at(i));

                flops[pid] ++;
            }
            

            // world.log("I am processor %d and my workstack has size %d", pid, work_stack.size()); 

            auto send_work              = bulk::queue<long long int, int[], long long int>(world);
            // auto send_done              = bulk::queue<int>(world);final_nodes
            auto send_work_stack_length = bulk::queue<long long int, int>(world);
            // auto send_work              = bulk::queue<int>(world);


            int done = false;
            while (!done) {
    //	    world.log("I am processor %d and I'm about to do some work", pid);//Mark
                if (work_stack.empty()) {
                    // world.sync();
                    done = true;
    //		world.log("I am processor %d and apparantly I didn't have any work", pid); //Mark
                } 
                else {
                    // world.log("processor %d does work starting node = %d, count = %d, path length = %d", pid, work.w, count, work.N);
                    saw(d, N, count, p, pid, work_stack, flops);
                    // world.log("I am processor %d and did some work, my workstack now has size %d", pid, work_stack.size()); //Mark
                }
                    // world.log("I am processor %d and it has been %d since I synced, I'll sync at %d !",pid,time_since_last_sync,SYNC_TIME);// Mark
                time_since_last_sync ++;

                flops[pid] ++;
                
                //We should sync if we are over the sync time to redistribute the work. 
                if (time_since_last_sync >= SYNC_TIME) {
                    syncs[pid] ++;
                    auto end_sync = chrono::steady_clock::now();
                    auto duration = chrono::duration_cast<chrono::milliseconds>(end_sync - start_sync).count();
                    start_sync = chrono::steady_clock::now();
                    auto current_time = chrono::duration_cast<chrono::milliseconds>(end_sync - begin).count();
                    // world.log("I am processor %d and I'm gonna sync", pid);
                    world.log("%lld, %d, %d, %d, %d, %d, %d", current_time, d, N, p, pid, work_stack.size(), duration);
                    //First we share how much work we have 
                    send_work_stack_lengths(p, pid, work_stack.size(), send_work_stack_length, flops);
                    // if (pid == p - 1){
                    //         world.log("-------------------------------------------------");
                    // }
                    world.sync();

                    //Then we receive the work.  
                    vector<long long int> work_stack_lengths(p, 0);
                    for (auto [remote_pid, remote_work_stack_length] : send_work_stack_length) {
                        work_stack_lengths[remote_pid]  = remote_work_stack_length;
                    }

                    // We check if we are done, which is the case if everyone has a work stack of size 0
                    done = true;
                    for (long long int i = 0 ; i < work_stack_lengths.size(); i ++){
                        if (work_stack_lengths[i] != 0 ){
                            done  = false;
                        }

                        flops[pid] ++;
                    }
                    if (done) {
                        // world.log("I am processor %d, I think we are done and I have found count %d", pid, count);
                        auto remote_counts = bulk::queue<long long int>(world);
                        remote_counts(0).send(count);
                        world.sync();
                        if (pid == 0){
                            long long int total = 0;
                            for (auto n : remote_counts) {
                                total += n; 

                                // flops[pid] ++;
                            }
                            // Multiply by 4d and add 2d for end result
                            // The adding of 2d corresponds to all paths which are | shaped
                            // The multiplying by 4d corresponds to mirroring (*2) and laying the starting segment in any axis (*d) in any direction (*2)
                            if (N != 0) {
                                // n_paths = total * 2 * d * d + 2 * d;
                                n_paths = total * 4 * d * (d - 1) + 2 * d;
                                // world.log("The total count is now %d", total * 4 * d + 2 * d);
                            }
                            else {
                                // world.log("There is only 1 path  of length 0, you shouldn't need a computer for this. ");
                            }
                        }
                        world.sync();
                    }

                    //Now, we calculate how the work should be redistributed.
                    // loads[pid].push_back(work_stack.size());
                    // world.log("%d", loads.at(pid).at(1));
                    map<long long int, int> shared_work = redistribute_work(work_stack_lengths, pid, p, flops);
                    // Then, we share the work we need to share
                    send_works(p, pid, work_stack, send_work, shared_work, flops);
                    world.sync();
                    //Finally we add the work we received  to our workstack 
                    
                    for (auto [v, visited, cur_N] : send_work) {
                        work_stack.push_back(work(v, vector<int>(visited), cur_N));
                        //SYNC_TIME = work_stack.size();
                        //time_since_last_sync = 0; 
                        // world.log("processor %d received starting node = %d, count = %d, cur_N = %d", pid, v, count, cur_N);
                    }
                    //Now we reset our timers 
                    SYNC_TIME = work_stack.size();
                    time_since_last_sync = 0; 
                    //we now should have workstacks of comparable size for each processor.
                }
                // world.log("I am processor %d and just have shared some work, my worstack has size %d", pid, work_stack.size());
            }
            // flops[pid] = flop;
        });
    }

    const auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    // cout << "It took " << duration << " ms and " << flops[0] << " flops on processor 0" << endl;

    // data to collect:
    // - runtime
    //  > sequential saw against p = 1, 2, 4, 8, 16, 32, 64, 128, 256
    // - # syncs
    //  > fix p, vary n 
    // - load before sync
    //  > min load, max load, desired load
    //  > bar plot maybe
    // - wall clock time in between SYNC_TIMEs
    // - # ops
    // remember: write to output pipe to file

    f_out.open("runtime.csv", ios_base::out);
    for (long long int i = 0; i < p; i ++) {
        f_out << d << ',' << N << ',' << p  << ',' << duration << ',' << i << ',' << flops.at(i) << ',' << syncs.at(i) << ',' << n_paths << endl;
        // long long int j = 0;
        // for (auto load : loads.at(i)) {
        //     j ++;
        //     f_out << j << ',' << i << ',' << load << endl;
        // }
    }
    f_out.close();
}

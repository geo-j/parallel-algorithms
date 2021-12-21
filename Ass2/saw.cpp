#include <chrono>
#include <fstream>
#include <numeric>
#include <vector>
#include <map>
#include <algorithm>
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
// TODO: use an adjacency list instead of a matrix
// TODO: make visited an integer instead of a  bool array
// TODO: Make worstack a stack instead of a vector?
// TODO: Make a neighbours function which can use a graph, but also a graph from suitesparse/ generate a square lattice by calculations so we save memory)
// DONE: only pass the workstack to saw
// DONE? make redistribute work cyclically but not by going trough everything (do clever divisions)


void saw(bulk::world &world, long long int n, long long int N, vector<vector<int>> A, long long int &count, long long int p, long long int pid, vector<work> &work_stack) {
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
    if (!task.visited.at(task.v)) {
	 // Then we check whether our path is of the needed length
         if (task.cur_path_length == N) {
            count ++;
        }
	// If not, we will add all extensions of the path to our workstack
        else {
            task.visited[task.v] = true;
            // for (long int w = 0; w < n; w ++) {
            //     if (A[task.v][w]) {
            for (auto w : A[task.v])
                    // We should take care to send a copy of visited as it will be different the next time we call it. 
                    work_stack.push_back(work(w, vector<int>(task.visited), task.cur_path_length + 1));
                // }
            // }
        }
    }
}

void send_work_stack_lengths(bulk::world &world, long long int p, long long int pid, int work_stack_length, bulk::queue<long long int, int> &send_work_stack_length) {
/* shares the lengths of the work stacks to all other processors
 */	
    for (long long int i = 0; i < p; i ++) {
        send_work_stack_length(i).send(pid, work_stack_length);
    }
}

void send_works(bulk::world &world, long long int p, long long int pid, vector<work> &work_stack, bulk::queue<long long int, int[], long long int> &send_work, map<long long int, int> shared_works) {
/* sends the work according to the redistribution method as below */
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

    // Our ideal workload is one where each processor has about the same amount of work, 
    vector<long long int> fair_work_stack_lengths(p, 0);
	long long int fair_workload        = total_workload / p;
	long long int procs_with_more_work = total_workload % p;
	
	for (long long int i = 0; i < p; i ++){
        fair_work_stack_lengths[i] = fair_workload;
        if (i < procs_with_more_work){
                fair_work_stack_lengths[i] ++;
        }
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
    for (long long int current_offloader = 0; current_offloader < pid + 1; current_offloader ++) {
        long long int current_receiver = 0;
        while (work_stack_lengths[current_offloader] - fair_work_stack_lengths[current_offloader] > 0) {    
			// as long as we still have work to offload we give work to anyone who needs it
            long long int receiver_needs = work_stack_lengths[current_receiver] - fair_work_stack_lengths[current_receiver];
            if (receiver_needs < 0) {    // for anyone who still needs work
                long long int can_offload = work_stack_lengths[current_offloader] - fair_work_stack_lengths[current_offloader];
                long long int offloaded;
				//We should not give away so much work that we don' t have any anymore
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
				// Now if we have given away work, we should keep track of it so we can actually do it later
                if (current_offloader == pid) {
                    sharing_map.insert(std::pair(current_receiver, offloaded));
                }
            }
            current_receiver ++;
        }
    }
    return sharing_map;
}
    



int main(int argc, char* argv[]) {
	//Starting the bulk system and taking user input
    bulk::thread::environment env;
    long long int n, N, v, p = env.available_processors(); // get available processors
    ofstream f_out;
    vector<long long int> flops(p);
    // vector<vector<long long int>> loads;
    vector<vector<int>> A;

    
    for (int i = 1; i < argc; i ++) {
        string arg = argv[i];
        if (arg == "-p") {  // use a flag for the number of processors. if none, then the maximum is chosen
            p = static_cast<long long int>(stoi(argv[++ i]));
        } else {
            cerr << "wrong arguments";
        }
    }

    // for (long long int i = 0; i < p; i ++) {
    //     vector<long long int> processor(1, 0);
    //     loads.push_back(processor);
    // }

    cin >> n;
    ifstream f_in("input/input_" + to_string(n));
    for (long long int i = 0; i < n; i ++) {
        vector<int> row(n);
        A.push_back(row);
        // adjacency_list.push_back(row);
        for (long int j = 0; j < n; j ++) {
            int edge;
            f_in >> edge;
            // A[i][j] = edge;
            if (edge)
                A[i].push_back(j);
        }
    }
    cin >> N >> v;
	cout << "n = "<< n << ", N = " << N << ", v = " << v << ", p = " << p << endl; 

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

	//Initialize processor 0 as the first one with work, no on else has any yet
        if (pid == 0){
            work_stack.push_back(work(v, visited, path_length_so_far));
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
                done = true;
//		world.log("I am processor %d and apparantly I didn't have any work", pid); //Mark
            } 
            else {
                // world.log("processor %d does work starting node = %d, count = %d, path length = %d", pid, work.w, count, work.N);
                saw(world, n, N, A, count, p, pid, work_stack);
		        // world.log("I am processor %d and did some work, my workstack now has size %d", pid, work_stack.size()); //Mark
            }
 	            // world.log("I am processor %d and it has been %d since I synced, I'll sync at %d !",pid,time_since_last_sync,SYNC_TIME);// Mark
            time_since_last_sync ++;
            
	        //We should sync if we are over the sync time to redistribute the work. 
            if (time_since_last_sync >= SYNC_TIME) { 
		        // world.log("I am processor %d and I'm gonna sync", pid);
                //First we share how much work we have 
                send_work_stack_lengths(world, p, pid, work_stack.size(), send_work_stack_length);
                if (pid == p - 1){
                        world.log("-------------------------------------------------");
                }
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
                }
                if (done) {
                    world.log("I am processor %d, I think we are done and I have found count %d", pid, count);
                    world.sync();
                }

                //Now, we calculate how the work should be redistributed.
                // loads[pid].push_back(work_stack.size());
                // world.log("%d", loads.at(pid).at(1));
                map<long long int, int> shared_work = redistribute_work(world, work_stack_lengths, pid, p);
                // Then, we share the work we need to share
                send_works(world, p, pid, work_stack, send_work, shared_work);
                world.sync();
                //Finally we add the work we received  to our workstack 
                 
                for (auto [v, visited, cur_N] : send_work) {
                    work_stack.push_back(work(v, visited, cur_N));
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
        flops[pid] = flop;
    });

    const auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    cout << "It took " << duration << " ms and " << flops[0] << " flops on processor 0" << endl;

    // f_out.open("loads2.csv", ios_base::app);
    // for (long long int i = 0; i < p; i ++) {
    //     // f_out << n << ',' << p  << ',' << duration << ',' << i << ',' << flops.at(i) << ',' << 'p' << endl;
    //     long long int j = 0;
    //     for (auto load : loads.at(i)) {
    //         j ++;
    //         f_out << j << ',' << i << ',' << load << endl;
    //     }
    // }
    f_out.close();
}
#include <numeric>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <cmath>

using namespace std;

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

vector<work> starting_L (long long int N, int d){
    /* Should give all paths going left i times and then upwards, in the counting as in the neighbour function.

    Input 
        N: maximal path length
        d: dimensions. 

    In a regular square lattice we can make use of symmetry to do less work. 
    We can assume all starting paths are of an L-shape: first we go in one direction i times, then we take one step in another direction. 
    Output: all starting work of such paths. 

    This symmetry should be counted 2*d amount of times. 
    There is also a path which is not L-shaped, but | -shaped, a straight line. This also happens 2d times, 
    Together the |-shape of size N and all the L-shaped give all starting paths modulo symmetry. 

    Thus we can distribute all the L-shaped starting paths cyclically, 
    initialize the count of one processor to 1 instead of 0 
    and multiply the end result by 2*d to get the total number of paths. 
      
    
    */
    vector<work> starting_positions;
    
    long long int centre = pow(N, d + 1) ;
    long long int startpos = centre + N + 2; 
    auto visited = vector <int> (2 * centre);
    visited[centre] = 1;

    for (int i = 1; i < N; i++){
        visited [centre + i] = 1;
        startpos ++ ; 
        work workload =  {startpos, vector<int>(visited), i};
        starting_positions.push_back(work(workload) );
    }
    return starting_positions;
}



vector<long long int> neighbours (long long int N, int d, long long int v){
    /* Input:
        d for the dimension
        N for the max size of the walk
        v for the node we need the d-dimensional neighbours of

        Output:
        a list of all neighbours of v in a d-dimensional grid of size N
        the centre of this grid should have coordinate pow(n,d+1). 
        Then in a diamond of size N around this centre, 
        the neigbhours should match up, and all have positive values. 
    */
    vector<long long int> neighbors;

    for (int i = 0 ; i<d; i++){
        neighbors.push_back(v + pow((N+2),i));
        neighbors.push_back(v - pow((N+2),i));
    }
    return neighbors;
}

// map<long long int, int> redistribute_work(vector <long long int> work_stack_lengths, long long int pid, long long int p) {
//     /*
//         Input ::
//         world, same as always
//         work_stack_lengths: for every processor we have the length of their work stack
//         pid:: which processor are we
//         p:: how many processors are there

//         Output::
//         A map from int to int, representing how much work this processor should send to each processor (so if the map is [(1,100), (4,20)] we'll send 100 work to processor 1 and 20 work to processor 4)
//     */

//     // cout << "here" << endl;
//     long long int total_workload = accumulate(work_stack_lengths.begin(), work_stack_lengths.end(), 0);
//     // cout << total_workload << endl;
//     //The best workload is sorta cyclically distributed
//     vector<long long int> fair_work_stack_lengths(p, 0);
    
//     // Our ideal workload is one where each processor has about the same amount of work, 
//     // This distribution is inspired by a cyclic work distribution, which we use to not deal with rounding errors. 
//     long long int current_pid = 0;
//     for (long long int i = 0; i < total_workload; i ++) {
//         fair_work_stack_lengths[current_pid] ++;
//         current_pid = (current_pid + 1) % p; 
//     } 
//     // for (auto len : fair_work_stack_lengths) {
//     //     cout << len << ',';
//     // }
//     // cout << endl;
  
//     // //for the moment notepad
//     // int workload_difference = work_stack_lengths[pid] - fair_work_stack_lengths[pid]; 
//     //     if (workload_difference > 0) {
//     //         //we should offload work
//     //     }
//     //     else{
//     //         // We should receive work 
//     //     }

//     map<long long int, int> sharing_map;
//     // //the following loop will go trough how the work should be offloaded by everyone 
//     // //when we are at our own processor and we see we should offload work, 
//     // // we'll keep track of it and actually put it in the map
//     //     // for (long long int current_offloader = 0; current_offloader < pid + 1; current_offloader ++) {
//     for (long long int current_offloader = 0; current_offloader < pid + 1; current_offloader ++) {
//         long long int current_receiver = 0;
//         while (work_stack_lengths[current_offloader] - fair_work_stack_lengths[current_offloader] > 0) {    // as long as we still have work to offload
//             long long int receiver_needs = work_stack_lengths[current_receiver] - fair_work_stack_lengths[current_receiver];
//             if (receiver_needs < 0) {    // for anyone who still needs work
//                 long long int can_offload = work_stack_lengths[current_offloader] - fair_work_stack_lengths[current_offloader];
//                 // cout << "processor " << current_offloader << " can offload " << can_offload << ' ' << "according to process " << pid << endl;
//                 // work_stack_lengths[current_offloader] --;

//                 long long int offloaded;
//                 if (abs(receiver_needs) > can_offload) {
//                     work_stack_lengths[current_offloader] -= can_offload;
//                     work_stack_lengths[current_receiver]  += can_offload;
//                     offloaded = can_offload;
//                 }
//                 else {
//                     work_stack_lengths[current_offloader] += receiver_needs;
//                     work_stack_lengths[current_receiver]  -= receiver_needs;
//                     offloaded = -receiver_needs;

//                 }
//                 if (current_offloader == pid) {
//                     sharing_map.insert(std::pair(current_receiver, offloaded));
//                     // cout << pid << " Wants to offload " << sharing_map.at(current_receiver) <<" to processor " << current_receiver << endl; 
//                 }
//             }
//             current_receiver ++;
//         }
//     }

//     return map<long long int, int>(sharing_map);
// }


int main(){
    int n = 2;
    int d = 2;
    for (auto w : starting_L(n,d) ){
        cout << "starting node" << w.v << ", path length" << w.cur_path_length<< endl;
        for (auto v : w.visited)
            cout << '\t' << v << ' ';
        cout << endl;
    }
    //     cout << "neighbours of " << i <<endl;
    // 
    // for (int i : neighbours(n,d,pow(2*n,d)) ){
    //     cout << "neighbours of " << i <<endl;
    //     for (int j : neighbours(n,d,i)){
    //         cout << j <<"," ;
    //     }
    //     cout <<"-------------------------"<<endl;
    // }
    // cout<< "neighbours of -1------"<< endl;
    // for (int i : neighbours(2,2,7)){
    //     cout << i << endl;
    // }
    // cout << "neighbours of 4------"<< endl;
    // for (int i : neighbours(2,2,12)){
    //     cout << i << endl;
    // }
    return 1; 
}

// int main(){
//     vector<long long int> example_distribution = {10,10,2,2};
//     for (long long int i = 0; i < 4 ; i++){
//         // map<long long int, int> redist
//         map<long long int, int> redist = redistribute_work(example_distribution, i, 4);
//         // cout << i << " Wants to offload " << redist.at(1) <<" to processor " << 2 << endl; 

//         for (std::pair<long long int, int> el : redist) {
//             cout << " processor " << i << " offloaded " << el.second << " to processor " << el.first << endl;

//         }
//         // for (auto [key, value] : redist){
//         // }
//     }
//     return 1; 
// }



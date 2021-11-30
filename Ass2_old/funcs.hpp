#include <vector>
#include <map>
#include <algorithm>
#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>

using namespace std;

struct workload {
    long int pid;
    long int cur_path_len;
    long int w;
    vector<int> visited;
    // long int count; 
};

bool compare_workload(workload const& lhs, workload const& rhs) {
    if (lhs.cur_path_len == rhs.cur_path_len) {
        return lhs.pid < rhs.pid;
    }
    else {  
        return lhs.cur_path_len < rhs.cur_path_len;
    }
}


map<long int, workload> transfer_work(bulk::world &world, vector<long int> have_nots , vector<workload> haves) {
    world.log("transfer work called");
    sort(have_nots.begin(), have_nots.end());
    sort(haves.begin(), haves.end(), &compare_workload);

    map<long int, workload> transfer_map;
    long int min_len = min(haves.size(), have_nots.size());
    for (long int i = 0; i < min_len; i ++)
        transfer_map.insert(std::make_pair(have_nots[i], haves[i]));

    return transfer_map;
}

pair<bool, workload> ask_for_work(bulk::world &world, long int p, long int pid) {
    /*
    This function is called by a processor which keeps sending the signal that it does not have any work
    If a processor broadcasts out that it does have work, we distribute the work available across all processors asking for work. 
    Input: the world, the number of processors, and the processor id of the processor without work
    Output: a tuple (int, workload) where the int equals 0 if there is no new work.
                                                         1 if there is new work.
                                                        -1 if everyone is done.
    and the workload is the new workload if there is new work. 
    */

    world.log("processor %d asks for work", pid);
    auto transfer_have_nots = bulk::queue<long int>(world);
    auto transfer_haves     = bulk::queue<long int, long int, long int, int[]>(world);
    vector<long int> have_nots;
    vector<workload> haves;
    // the haves and have nots will be the list of processors which have work and a list of processors which don't have work.     
    

    for (long int i = 0 ; i < p; i ++) {
        if (pid != i) {
            transfer_have_nots(i).send(pid);
            world.log("processor %d says it needs work to %d", pid,i);
        }
    }
    world.log("\task for work midway", pid);
    world.sync();
    world.log("\tafter ask for work sync", pid);
    //first we convert the transfered queues to vectors
    // for (auto [remote_pid, remote_cur_path_len, remote_w, remote_visited] : transfer_haves){
    //     workload remote_workload = {remote_pid, remote_cur_path_len, remote_w, remote_visited}; 
    //     haves.push_back(remote_workload);
    // }
    for (auto have_not : transfer_have_nots){
        world.log("have not: %d", have_not);
        have_nots.push_back(have_not);
    }
    //

    if (haves.empty()) {
        world.log("---- haves empty");
        int have_new_work = -1;
        workload new_work;//= new workload();
        
        return std::make_pair(have_new_work, new_work);
    }
    
    int have_new_work = 0;
    workload new_work ;//= new workload();

    map<long int, workload> transfer_map = transfer_work(world, have_nots, haves);
    if (transfer_map.contains(pid)) {
        world.log("processer %d thinks it has new work ", pid);
        have_new_work = 1;
        workload new_work = transfer_map.at(pid);
    }

    world.log("processor %d asks for work end", pid);

    return std::make_pair(have_new_work, new_work);
}

int redistribute_work(bulk::world &world, long int p, long int pid, workload my_workload, int sync_time, int &time_since_last_sync) {
    world.log("processor %d wants to redistribute work", pid);
    world.log("processor %d has synced %d steps ago", pid, time_since_last_sync);
    time_since_last_sync ++;
    if (time_since_last_sync > sync_time){
        world.log("if-statement entered");
        time_since_last_sync    = 0;
        auto transfer_have_nots = bulk::queue<long int>(world);
        auto transfer_haves     = bulk::queue<long int, long int, long int, int[]>(world);
        vector<long int> have_nots;
        vector<workload> haves;
        // for (long int i = 0 ; i < p; i ++) {
        //     world.log("my_workload redistribute: pid = %d, cur_path_len = %d, w = %d, visited[w] = %d, %d to %d", my_workload.pid, my_workload.cur_path_len, my_workload.w, my_workload.visited[my_workload.w], time_since_last_sync, pid);
        //     transfer_haves(i).send(my_workload.pid, my_workload.cur_path_len, my_workload.w, my_workload.visited);
        // } 
        world.log("\tredistribute work midway");
        world.sync();
        world.log("\tsize of transfer_have_nots: %d", transfer_have_nots.size());
        
        for (auto have_not: transfer_have_nots){
            have_nots.push_back(have_not);
        }

        // for (auto [remote_pid, remote_cur_path_len, remote_w, remote_visited] : transfer_haves){
        //     world.log("whaaa");
        //     workload remote_workload = {remote_pid, remote_cur_path_len, remote_w, remote_visited}; 
        //     haves.push_back(remote_workload);
        // }

        // int offloaded_my_work = false; 

        map<long int, workload> transfer_map = transfer_work(world, have_nots, haves);
        for (auto const& [key, value] : transfer_map) {
            if (value.pid == pid) {
                world.log("\twork redistributed to %d", pid);
                return true;
            }
        }
    }
    world.log("\tno work redistributed");
    return false;
}    

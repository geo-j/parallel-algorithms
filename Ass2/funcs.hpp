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


map<long int, workload> transfer_work(vector<long int> have_nots , vector<workload> haves) {
    sort(have_nots.begin(), have_nots.end());
    sort(haves.begin(), haves.end(), &compare_workload);

    map<long int, workload> transfer_map;
    long int min_len = min(haves.size(), have_nots.size());
    for (long int i = 0; i < min_len; i ++)
        transfer_map.insert(std::make_pair(have_nots[i], haves[i]));

    return transfer_map;
}

pair<bool, workload> ask_for_work(bulk::world &world, long int p, long int pid) {
    auto transfer_have_nots = bulk::queue<long int>(world);
    auto transfer_haves     = bulk::queue<long int, long int, long int, int[]>(world);
    vector<long int> have_nots;
    vector<workload> haves;

    for (long int i = 0 ; i < p; i ++) {
        transfer_have_nots(i).send(pid);
    }
    world.sync();
    for (auto [remote_pid, remote_cur_path_len, remote_w, remote_visited] : transfer_haves){
        workload remoteworkload = {remote_pid, remote_cur_path_len, remote_w, remote_visited}; 
        haves.push_back(remoteworkload);
    }
    for (auto have_not : transfer_have_nots){
        have_nots.push_back(have_not);
    }
    
    bool have_new_work = false;
    workload new_work;

    map<long int, workload> transfer_map = transfer_work(have_nots, haves);
    if (transfer_map.contains(pid)) {
        have_new_work = true;
        workload new_work = transfer_map.at(pid);
    }

    return std::make_pair(have_new_work, new_work);
}


bool redistribute_work(bulk::world &world, long int p, long int pid, workload my_workload, int sync_time, int &time_since_last_sync) {
    time_since_last_sync ++;
    if (time_since_last_sync > sync_time){
        time_since_last_sync    = 0;
        auto transfer_have_nots = bulk::queue<long int>(world);
        auto transfer_haves     = bulk::queue<long int, long int, long int, int[]>(world);
        vector<long int> have_nots;
        vector<workload> haves;
        for (long int i = 0 ; i < p; i ++) {
            transfer_haves(i).send(my_workload.pid, my_workload.cur_path_len, my_workload.w, my_workload.visited);
        } 
        world.sync();
        
        for (auto [remote_pid, remote_cur_path_len, remote_w, remote_visited] : transfer_haves){
            workload remoteworkload = {remote_pid, remote_cur_path_len, remote_w, remote_visited}; 
            haves.push_back(remoteworkload);
        }
        for (auto have_not: transfer_have_nots){
            have_nots.push_back(have_not);
        }

        bool offloaded_my_work = false; 

        map<long int, workload> transfer_map = transfer_work(have_nots, haves);
        for (auto const& [key, value] : transfer_map) {
            if (value.pid == pid)
                return true;
        }
    }
    return false;
}    

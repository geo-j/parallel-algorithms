#include <chrono>
#include <fstream>
#include "funcs.hpp"

using namespace std;

const int SYNC_TIME = 100;

void saw(bulk::world &world, int &time_since_last_sync, long int n, long int N, long int v,  vector<vector<bool>> A, long int cur_path_len, vector<int> visited, vector<long int> walk, long int &count, long int p, long int pid) {
    world.log("processor %d at cur_path_len %d and vector %d", pid, cur_path_len, v);
    // cout << i << ' ' << v + 1 << endl;
    if (!visited.at(v)) {
        if (cur_path_len == N) {
            for (long int j = 0; j < walk.size(); j ++) {
                // cout << walk.at(j) + 1 << ", ";
                world.log("%d,", walk.at(j));
            }
            count ++;
            // cout << endl;
        }
        else {
            visited[v] = true;
            for (long int w = 0; w < n; w ++) { // for all neighbours in the adjacency matrix A
                if (A[v][w]) {
                    workload my_workload = {pid, cur_path_len, w, visited};
                    bool offloaded_work = redistribute_work(world, p, pid, my_workload, SYNC_TIME, time_since_last_sync);
                    walk.push_back(w);               
                    if (!offloaded_work) {
                        saw(world, time_since_last_sync, n, N, w, A, cur_path_len + 1, visited, walk, count, p, pid);
                    }
                    walk.pop_back();
                }
            }
            visited[v] = false;
        }
    }
// Below should be looped somehow. 

    auto pair =  ask_for_work(world, p, pid);

    
    while (!pair.first) {
        cur_path_len = pair.second.cur_path_len;
        int w        = pair.second.w;
        visited      = pair.second.visited;
        time_since_last_sync = 0;
        saw(world, time_since_last_sync, n, N, w, A, cur_path_len + 1, visited, walk, count, p, pid);
    };
   
}



int main(int argc, char* argv[]) {
    bulk::thread::environment env;
    long int n, N, v, p = env.available_processors();
    ofstream f_out;
    vector<long int> flops(p);
    vector<vector<bool>> A;

    for (int i = 1; i < argc; i ++) {
        string arg = argv[i];
        if (arg == "-p") {
            p = static_cast<long int>(stoi(argv[++ i]));
        } else {
            cerr << "wrong arguments";
        }
    }

    cin >> n;
    ifstream f_in("input_" + to_string(n));
    for (long int i = 0; i < n; i ++) {
        vector<bool> row(n);
        A.push_back(row);
        for (long int j = 0; j < n; j ++) {
            bool edge;
            f_in >> edge;
            A[i][j] = edge;
        }
    }
    cin >> N >> v;

    const auto start = chrono::steady_clock::now();

    env.spawn(p, [&n, &p, &flops, &N, &v, &A](auto& world) {
        // init local processors
        auto pid = world.rank(); // local processor ID
        long int flop = 0;
        vector<int> visited(n, false);
        long int count = 0;
        int time_since_last_sync = SYNC_TIME - 1;
        vector<long int> walk;
        walk.push_back(v);

        if (pid == 0){
            saw(world, time_since_last_sync, n, N, v, A, 0, visited, walk, count , p, pid);
        } else {
            auto pair =  ask_for_work(world, p, pid);
            if (pair.first) {
                long int cur_path_len = pair.second.cur_path_len;
                int w        = pair.second.w;
                visited      = pair.second.visited;
                int time_since_last_sync = 0;
                saw(world, time_since_last_sync, n, N, w, A, cur_path_len + 1, visited, walk, count, p, pid);
            };
        world.log("%d", count);
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

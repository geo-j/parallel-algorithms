#include <chrono>
#include <fstream>
#include "funcs.hpp"

using namespace std;

const int SYNC_TIME = 100;

void saw(bulk::world &world, int &time_since_last_sync, long int n, long int N, long int v,  vector<vector<int>> A, long int cur_path_len, vector<int> visited, vector<long int> walk, long int &count, long int p, long int pid, int top) {
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

    // send done to everyone
    // receive done and see if you can redistribute work

    // share status
    // receive messages

    // int done = false;
    if (!done){
        if (!visited.at(v)) {
            if (i == N) {
                world.log("found path:");
                count ++;
                for (long int j = 0; j < walk.size(); j ++) {
                    world.log("%d", walk.at(j) + 1);
                }
                // count ++;
                // cout << endl;
            }
            else {
                vector<int> workstack;
                for (long int w = 0; w < n; w ++) {
                    if (A[v][w]) {
                        workstack.push_back(w);}
                }
            }
            done = false;
            if(top && worstack.empty()) {
                done = true;
            }
    }

        // communicate done, workstack 
        world.sync();
        // receive status from everyone
        if (everyones votes done){
            exit_saw;
        }
        else{
            // decide whether you get new work or lose work -- redistribute the work called redundantly 

            // call saw recursively on your new work
            visited[v] = true;
            for (workstack) {
                walk.push_back(w);
                saw(n, N, w, A, i + 1, visited, walk, count);
                walk.pop_back();
            }
            visited[v] = false;
        }

    }


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
            p = static_cast<long int>(stoi(argv[++ i]));
        } else {
            cerr << "wrong arguments";
        }
    }

    cin >> n;
    ifstream f_in("input_" + to_string(n));
    for (long int i = 0; i < n; i ++) {
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
        long int flop = 0;
        vector<int> visited(n, false);
        long int count = 0;
        int time_since_last_sync = SYNC_TIME - 2;
        vector<long int> walk;
        walk.push_back(v);

        saw(world, time_since_last_sync, n, N, v, A, 0, visited, walk, count, p, pid, true);
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

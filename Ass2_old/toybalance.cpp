#include <chrono>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>

using namespace std;

const int SYNC_TIME = 100;

int process (bulk::world &world, int &time_since_last_sync,  long int &count, long int p, long int pid, int begin, int end){
  for (i=begin, i<end, i++){
      auto offloaded_work = redistribute_work(world,pid,i);
      if (offloaded_work.first == 0){
          world.log("%d processed by %d",i,pid)
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


        if (pid != 0) {
            auto pair =  ask_for_work(world, p, pid);
        }
        // flops[pid] = flop;
        else {
            process(world, time_since_last_sync, p, pid,1,1000);
            world.log("finished saw processor 0");
        } 
      
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

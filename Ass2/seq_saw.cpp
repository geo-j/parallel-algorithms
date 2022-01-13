#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <chrono>

using namespace std;

/* Description: Function that recursively counts all SAWs of length N in a simple undirected graph
*  Returns: nothing
*  input param n: number of vertices in the graph
*  input param N: length of the SAW
*  input param v: vertex to start the SAWs in
*  input param A: adjacency matrix of the graph
*  param i: current length of the SAW, has to be initialised with 0 when calling the function
*  param visited: boolean array tracking the visited nodes in the current SAW, initialised with all false when calling the function
*  param walk: vector containing the vertices of the current SAW in visiting order, initialised with v when calling the function
*  output param count: the number of SAWs in the graph, initialised with 0 when calling the function

*/

vector<long long int> neighbours(long long int N, int d, long long int v){
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
	// cout<< "neighbours called on " << N<< ", "  << d <<"," << v; 
    vector<long long int> neighbors;

    for (int i = 0 ; i < d; i ++){
        neighbors.push_back(v + pow((N + 2), i));
        neighbors.push_back(v - pow((N + 2), i));
    }
    return neighbors;
}

void saw(long int n, long long int d,  long int N, long int v,  vector<vector<bool>> A, long int i, vector<bool> visited, vector<long int> walk, long int &count, long int &flops) {
    // cout << i << ' ' << v + 1 << endl;
    if (!visited.at(v)) {
         if (i == N) {
            // for (long int j = 0; j < walk.size(); j ++) {
                // cout << walk.at(j) + 1 << ", ";
            // }
            count ++;
            // cout << endl;
        }
        else {
            visited[v] = true;
			for (auto w : neighbours (N, d, v)) {
				walk.push_back(w);
				saw(n, d, N, w, A, i + 1, visited, walk, count, flops);
                walk.pop_back();
			}
//            for (long int w = 0; w < n; w ++) {
//                if (A[v][w]) {
//                   walk.push_back(w);
//                    saw(n, N, w, A, i + 1, visited, walk, count);
//                    walk.pop_back();
//                }
//            }
            visited[v] = false;
        }
    }
}

int main() {
    long int n, d, N, v, count = 0, flops = 0;
    vector<long int> walk;
    vector<vector<bool>> A;
    ofstream f_out;
//    cin >> n;
//    ifstream f("input_" + to_string(n));
//    for (long int i = 0; i < n; i ++) {
//        vector<bool> row(n);
//        A.push_back(row);
//        for (long int j = 0; j < n; j ++) {
//            bool edge;
//            f >> edge;
//            A[i][j] = edge;
//        }
//    }
    cin >> N >> d;
	v = pow(N+2,d); 
	n = 2*v ; 
    const auto start = chrono::steady_clock::now();
    vector<bool> visited(n , false);
    walk.push_back(v);
    saw(n, d, N, v, A, 0, visited, walk, count, flops);
    const auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    // cout << count << endl;
    f_out.open("runtime.csv", ios_base::app);
    // for (long long int i = 0; i < p; i ++) {
    f_out << d << ',' << N << ',' << 0  << ',' << duration << ',' << 0 << ',' << flops << ',' << 0 << ',' << count << ',' << 0 << endl;
        // long long int j = 0;
        // for (auto load : loads.at(i)) {
        //     j ++;
        //     f_out << j << ',' << i << ',' << load << endl;
        // }
    // }
    f_out.close();
    return 0;
}

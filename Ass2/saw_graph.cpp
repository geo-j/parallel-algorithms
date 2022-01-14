#include <iostream>
#include <fstream>
#include <vector>

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
void saw(long int n, long int N, long int v,  vector<vector<bool>> A, long int i, vector<bool> visited, vector<long int> walk, long int &count) {
    // cout << i << ' ' << v + 1 << endl;
    if (!visited.at(v)) {
         if (i == N) {
            for (long int j = 0; j < walk.size(); j ++) {
                cout << walk.at(j) + 1 << ", ";
            }
            count ++;
            cout << endl;
        }
        else {
            visited[v] = true;
            for (long int w = 0; w < n; w ++) {
                if (A[v][w]) {
                    walk.push_back(w);
                    saw(n, N, w, A, i + 1, visited, walk, count);
                    walk.pop_back();
                }
            }
            visited[v] = false;
        }
    }
}

int main() {
    long int n, N, v, count = 0;
    vector<long int> walk;
    vector<vector<bool>> A;

    cin >> n;
    cout << n << endl;
    // ifstream f("input_" + to_string(n));
    for (long int i = 0; i < n; i ++) {
        vector<bool> row(n);
        A.push_back(row);
        for (long int j = 0; j < n; j ++) {
            bool edge;
            cin >> edge;
            cout << edge << ' ';
            A[i][j] = edge;
        }
    }
    cin >> N >> v;
    // v = (N + 2) * (N + 2) + N + 2;
    cout << N << ' ' << v << endl;
    vector<bool> visited(n, false);
    walk.push_back(v);

    saw(n, N, v, A, 0, visited, walk, count);
    cout << count << endl;
    return 0;
}
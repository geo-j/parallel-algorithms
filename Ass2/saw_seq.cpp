#include <iostream>
#include <vector>

using namespace std;

void saw(long int n, long int i, long int xi, long int yi, vector<vector<bool>> visited, vector<pair<long int, long int>> walk, long int &count) {
    // cout << i << ' ' << xi << ' ' << yi << endl;
    if (!visited.at(xi).at(yi)) {
        if (i == n) {
            for (long int j = 0; j < walk.size(); j ++) {
                cout << "(" << walk.at(j).first << ", " << walk.at(j).second << ")";
            }
            count ++;
            cout << endl;
        }
        else {
            visited[xi][yi] = true;
            if (xi + 1 < n * n) {
                walk.push_back(make_pair(xi + 1, yi));
                saw(n, i + 1, xi + 1, yi, visited, walk, count);
                walk.pop_back();
            }
            if (xi - 1 >= 0) {
                walk.push_back(make_pair(xi - 1, yi));
                saw(n, i + 1, xi - 1, yi, visited, walk, count);
                walk.pop_back();
            }
            if (yi + 1 < n * n) {
                walk.push_back(make_pair(xi, yi + 1));
                saw(n, i + 1, xi, yi + 1, visited, walk, count);
                walk.pop_back();
            }
            if (yi - 1 >= 0) {
                walk.push_back(make_pair(xi, yi - 1));
                saw(n, i + 1, xi, yi - 1, visited, walk, count);
                walk.pop_back();
            }
            visited[xi][yi] = false;

        }
    }
}

int main() {
    long int n, count = 0;
    vector<vector<bool>> visited;
    vector<pair<long int, long int>> walk;

    cin >> n;
    for (long int i = 0; i < n * n; i ++) {
        vector<bool> row(n * n, false);
        visited.push_back(row);
    }

    saw(n, 0, n, n, visited, walk, count);
    cout << count << endl;
    return 0;
}
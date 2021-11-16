#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

using namespace std;

int main() {
    long int n;
    vector<vector<bool>> A;
    cin >> n;
    ofstream f("input_" + to_string(n));
    f << n * n << endl;

    for (long int i = 0; i < n * n; i ++) {
        vector<bool> row(n * n, false);
        A.push_back(row);
    }

    for (long int i = 0; i < n * n; i ++) {        
        for (long int j = i + 1; j < n * n; j ++) {
            cout << i << ' ' << j << endl;
            // cout << (j == i + 1) << ' ' << (j == i + n) << ' ' << (j % n >= 0) << ' ' << (j % n < n - 1) << endl;
            if ((j == i + 1 || j == i + n) && (j % n >= 0 && j % n <= n - 1) && !(i % n == n - 1 && j % n == 0)) {
                // cout << '\t' << "true" << endl;
                A[i][j] = true;
                A[j][i] = true;
            }
        }
    }

    for (long int i = 0; i < n * n; i ++) {
        for (long int j = 0; j < n * n; j ++) {
            f << A[i][j] << ' ';
        }
        f << endl;
    }


    return 0;
}
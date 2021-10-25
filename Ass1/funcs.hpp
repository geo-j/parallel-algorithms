#include <vector>
#include <map>
#include <math.h>

using namespace std;

pair<vector<int>, vector<int>> local_sieve(int s, int b, int N, int &flops) {
    int list_len =  N / b + 1;
    int sqrtN = sqrt(N) + 1;
    int number = s;

    flops += 4;
    
    vector<int> local_prime_bools(list_len, 1);
    if (s == 1) {
        local_prime_bools[0] = 0;
    }

    vector<int> local_primes;

    for (int k = 0; k < list_len; k ++) {
        if (local_prime_bools.at(k)) {
            if (number < sqrtN){
                local_primes.push_back(number);}
            
            flops ++;
            for (int j = k + number; j <= list_len; j += number) {
                local_prime_bools[j] = 0;

                flops ++;
            }
        }
        number += b;

        flops ++;
    }

    return make_pair(local_prime_bools, local_primes);
}
    

pair<vector<int>, vector<int>> b_coprimes(int b, int pid, int p, int &flops) {
    vector<int> s_candidates(b, true);
    vector<int> my_s, s_winners;
    s_winners.push_back(1);
    if (pid == 0) {
        my_s.push_back(1);
    }

    int processor_number = 1;

    for (int s = 2; s < b; s ++) {
        if (s_candidates.at(s)) {
            if (b % s == 0) {
                flops += 2;
                for (int j = s + s; j < b; j += s) {
                    s_candidates[j] = 0;

                    flops ++;
                }
            }
            else {
                if (processor_number == pid) {
                    my_s.push_back(s);
                }
                s_winners.push_back(s);
                processor_number ++; 
                processor_number = processor_number % p;

                flops += 2;
            }
        }
        flops ++;
    }

    return make_pair(my_s, s_winners);
}


map<int, int> inverse_dict (int b, int s, vector<int> k_list, int &flops) {
    map<int, int> inverse_dict_s;

    for (auto k : k_list) {
        for (auto v : k_list) {
            if ((k * v) % b == s) {
                flops ++;
                inverse_dict_s.insert({k, v});
                continue;
            }
        }
    }
    return inverse_dict_s;
}

void remove_multiples(int s, int b , int a, int sa, vector<int>& local_prime_bools,  map<int, int> inverse_dict_s, int &flops) {
    // int sa = a mod b, but this is better passed along in application.  
    if (sa == s){
        return;
    }
    int l = inverse_dict_s[sa];
    int first_multiple = l * a;
    int starting_index = (first_multiple - s) / b;
    int list_len = local_prime_bools.size();

    flops += 3;
    for (int i = starting_index; i < list_len; i += a){
        local_prime_bools[i] = false;
        flops ++;
    }
}


#include <vector>
#include <map>
#include <math.h>

using namespace std;

pair<vector<size_t>, vector<size_t>> local_sieve(size_t s, size_t b, size_t N, size_t &flops) {
    size_t list_len =  N / b + 1;
    size_t sqrtN = sqrt(N) + 1;
    size_t number = s;

    flops += 4;
    
    vector<size_t> local_prime_bools(list_len, 1);
    if (s == 1) {
        local_prime_bools[0] = 0;
    }

    vector<size_t> local_primes;

    for (size_t k = 0; k < list_len; k ++) {
        if (local_prime_bools.at(k)) {
            if (number < sqrtN){
                local_primes.push_back(number);}
            
            flops ++;
            for (size_t j = k + number; j < list_len; j += number) {
                local_prime_bools[j] = 0;

                flops ++;
            }
        }
        number += b;

        flops ++;
    }

    return make_pair(local_prime_bools, local_primes);
}
    

pair<vector<size_t>, vector<size_t>> distribute_small_numbers(size_t b, size_t pid, size_t p, size_t &flops) {
    vector<size_t> s_candidates(b, true);
    vector<size_t> my_s, s_winners;
    s_winners.push_back(1);
    if (pid == 0) {
        my_s.push_back(1);
    }

    size_t processor_number = 1;

    for (size_t s = 2; s < b; s ++) {
        if (s_candidates.at(s)) {
            if (b % s == 0) {
                flops += 2;
                for (size_t j = s + s; j < b; j += s) {
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


map<size_t, size_t> inverses (size_t b, size_t s, vector<size_t> k_list, size_t &flops) {
    map<size_t, size_t> inverse_s;

    for (auto k : k_list) {
        for (auto v : k_list) {
            if ((k * v) % b == s) {
                flops ++;
                inverse_s.insert({k, v});
                continue;
            }
        }
    }
    return inverse_s;
}

void remove_multiples(size_t s, size_t b , size_t a, size_t sa, vector<size_t>& local_prime_bools,  map<size_t, size_t> inverse_dict_s, size_t &flops) {
    // size_t sa = a mod b, but this is better passed along in application.  
    if (sa == s){
        return;
    }
    size_t l = inverse_dict_s[sa];
    size_t first_multiple = l * a;
    size_t starting_index = (first_multiple - s) / b;
    size_t list_len = local_prime_bools.size();

    flops += 3;
    for (size_t i = starting_index; i < list_len; i += a){
        local_prime_bools[i] = false;
        flops ++;
    }
}


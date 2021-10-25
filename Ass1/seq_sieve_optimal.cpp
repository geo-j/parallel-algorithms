#include <vector>
#include <math.h>
#include <iostream>

int index_to_number(int i) {
	return 2 * (i + 1) + 1;
}
int number_to_index (int n) { 
    return n / 2 - 1;
}


vector<int> primes_up_to(int N, int &flops) {
	int list_len = N / 2;
	flops ++; 
	vector<int> prime_bools (list_len, 1);

	for (int i = 0; i < list_len ; i++){
		if (prime_bools.at(i)){
			int n = index_to_number(i);
			flops += 6;
			for (int j = number_to_index( n * n ) ; j < list_len; j += n){
				prime_bools[j] = 0;
				flops ++;
			}
		}
		flops ++;

	}
	
	vector<int> primes;
	primes.push_back(2);
	for (int i = 0; i < prime_bools.size(); i ++){
		if (primes.at(i)) {
            primes.push_back(index_to_number(i));
			flops += 4;
        }
	} 
	return primes;
}

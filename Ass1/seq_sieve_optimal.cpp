#include <vector>
#include <math.h>
#include <iostream>

size_t index_to_number(size_t i) {
	return 2 * (i + 1) + 1;
}
size_t number_to_index (size_t n) { 
    return n / 2 - 1;
}


vector<size_t> primes_up_to(size_t N, size_t &flops) {
	size_t list_len = N / 2;
	flops ++; 
	vector<size_t> prime_bools (list_len, 1);

	for (size_t i = 0; i < list_len ; i++){
		if (prime_bools.at(i)){
			size_t n = index_to_number(i);
			flops += 6;
			for (size_t j = number_to_index( n * n ) ; j < list_len; j += n){
				prime_bools[j] = 0;
				flops ++;
			}
		}
		flops ++;

	}
	
	vector<size_t> primes;
	primes.push_back(2);
	for (size_t i = 0; i < prime_bools.size(); i ++){
		if (primes.at(i)) {
            primes.push_back(index_to_number(i));
			flops += 4;
        }
	} 
	return primes;
}

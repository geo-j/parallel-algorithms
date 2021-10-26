#include <vector>
#include <math.h>
#include <iostream>


vector<size_t> primes_up_to(size_t N, size_t &flops) {
	size_t list_len  = N+1;
	vector<size_t> primes;
	vector<size_t> prime_bools (list_len, 1);

	for (size_t p = 2; p <= sqrt(N); p ++) {
		if (prime_bools.at(p)) {
			primes.push_back(p);
			for (size_t i=p*p; i<= N; i+=p){
				prime_bools[i] = 0;
			}
		}
	}
	for (size_t p = sqrt(N)+1; p<=N; p++) {
		if (prime_bools.at(p)){
			primes.push_back(p);
		}
	}
	
	return primes;
}

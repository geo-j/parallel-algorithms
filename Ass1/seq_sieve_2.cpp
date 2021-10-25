#include <vector>
#include <math.h>
#include <iostream>

int index_to_number(int i) {
	return 2* (i+1) + 1;
}
int number2index (int n) { 
    return n/2 - 1;
}


vector<int> primesupto(int N, bulk::world& world) {
	int listlength = N/2;
	vector<int> primes (listlength, 1);
	for (int i = 0; i < listlength ; i++){
		if (primes.at(i)){
			int n = index_to_number(i);
			for (int j = number2index( n * n ) ; j < listlength ;j += n){
				primes[j] = 0;
			}
		}

	}
	
	vector<int> primenumbers;
	primenumbers.push_back(2);
	// int number = 3;
	for (int i = 0; i < primes.size(); i ++){
		if (primes.at(i)) {
            primenumbers.push_back(index_to_number(i));
        }
		// number += 2;
	} 
	return primenumbers;
}

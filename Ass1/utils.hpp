// This file contains the calculation and communication functions used by the main parallel sieving program

#include <math.h>

// Calculation
size_t gcd(size_t a, size_t b, size_t& flop) {
   if (a == 0 || b == 0) {
        flop += 2;
        return 0;
   }
   else if (a == b) {
        flop ++;
        return a;
   }
   else if (a > b) {
        flop += 2;
        return gcd(a - b, b, flop);
   }
   else {
       flop ++;
       return gcd(a, b - a, flop);
   }
}

// calculate the number based on its index in a cyclically distributed boolean array
size_t index_to_number(size_t i, size_t pid, size_t p, size_t &flop) {
    flop += 3;
    return pid + i * p + 1;
}

// calculate the index of the number in a cyclically distirbuted boolean array
size_t number_to_index(size_t number, size_t p, size_t pid, size_t &flop) {
    // return static_cast<int>(round(number / (p * 1.0)));
    return ceil((number - 1 - pid) / (p * 1.0));
}


// Communication
// function that sends a number to all the other processors
void put_numbers_to_all(size_t number, int pid, int p, bulk::queue<size_t> &q) {
    for (int i = 0; i < p; i ++) {
        if (i != pid) { 
            q(i).send(number);
        }
    }
}

// function that sends the locally found primes to only the processors that might have multiples of it
void put_prime_multiples(size_t prime, size_t n, int p, bulk::queue<size_t> &q, size_t& flop, bulk::world& world) {
    flop ++;
    for (size_t i = prime * 2; i <= n; i += prime) {
        // world.log("send prime %d to processor %d", prime, (i - 1) % p);
        q((i - 1) % p).send(i);
        flop += 3;
    }
}
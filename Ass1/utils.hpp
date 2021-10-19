// This file contains the calculation and communication functions used by the main parallel sieving program

#include <math.h>

// Calculation
int gcd(int a, int b, int& flop) {
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
int index_to_number(int i, int pid, int p, int &flop) {
    flop += 3;
    return pid + i * p + 1;
}

// calculate the index of the number in a cyclically distirbuted boolean array
int number_to_index(int number, int p, int pid, int &flop) {
    // return static_cast<int>(round(number / (p * 1.0)));
    return ceil((number - 1 - pid) / (p * 1.0));
}


// Communication
// function that sends a number to all the other processors
void put_numbers_to_all(int number, int pid, int p, bulk::queue<int> &q) {
    for (int i = 0; i < p; i ++) {
        if (i != pid) { 
            q(i).send(number);
        }
    }
}

// function that sends the locally found primes to only the processors that might have multiples of it
void put_prime_multiples(int prime, int n, int p, bulk::queue<int> &q, int& flop, bulk::world& world) {
    flop ++;
    for (int i = prime * 2; i <= n; i += prime) {
        // world.log("send prime %d to processor %d", prime, (i - 1) % p);
        q((i - 1) % p).send(i);
        flop += 3;
    }
}
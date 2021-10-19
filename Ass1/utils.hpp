// This file contains the calculation function used by the main parallel sieving program

#include <math.h>

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
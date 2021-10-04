/* Exercise 1.7
a) We can stop whenever we run out of memory, or we have crossed out all the numbers smaller than n
c) The probability of a number i to be prime is 1/(ln i) => here are n/(ln i) prime numbers smaller than n
The kth prime number is approximately kln k (https://proofwiki.org/wiki/Approximate_Value_of_Nth_Prime_Number)
We can find the number of total operations by summing over all the probabilities of numbers up to n to be a prime: \sum_{i <= n, i prime} 1/i
This sum is approximately the same with summing approximately over all the first n/(ln n) prime numbers, each approximately kln k, with a probability of 1/x of existing: \sum_{k = 2}^{n/ln n} 1/(kln k). This sum can be approximated using its integral, which results in ln ln n. Thus, there are approximately nln ln n operations to be done for each prime number smaller than n
Since there are approximately sqrt(n) prime numbers smaller than n, this results in nln ln n + sqrt(n) operations. 
https://cp-algorithms.com/algebra/sieve-of-eratosthenes.html
b)
*/


#include <iostream>
#include <vector>

using namespace std;

long long MAX_SIZE = 1000000;
int ops = 0;

bool isPrime(int n){    // O(sqrt(n)) => sqrt(n) ops worst case
    if (n < 2){
        return false;
    }

    if (n == 2 || n == 3){
        return true;
    }

    for (int i = 2; i * i <= n; i ++){
        if (n % i == 0)
            return false;

    }

    return true;
}

int main(){
    int n;
    vector<bool>primes(MAX_SIZE, true);

    cin >> n;

    for (int i = 2; i < n; i ++){
        ops ++;
        if (isPrime(i))     // count checking whether a number is prime as one operation
            for (int j = i * i; j <= n; j += i){    // n/i operations for any prime i
                ops ++;
                primes.at(j) = false;
            }
    }
    
    for (int i = 2; i < n; i ++)
        if (primes.at(i))
            cout << i << ' ';
    cout << endl << ops << endl;

    return 0;
}

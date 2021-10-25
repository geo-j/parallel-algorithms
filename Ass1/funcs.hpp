#include <vector>
#include <map>

using namespace std;
// localSieve(s,b,N):
// % input : three numbers s,b,N with:
// %         s<b<N
// %         gcd(s,b)=1
// %         b|N // To simplify the list length
// % output: one Boolean array and one list of numbers. 

// %         The Boolean array represents all numbers s+kp, where k is the index, 
// %         it gives true if the number is relatively prime.
        
// %         The list gives all of these relative primes. 

// array(type=bool, length = N/b) localarray     := <True> ;
// list(int)                      localprimelist := [ ]    ;

// number := s;
// for (k:= 0, k <= n/b, k++):
//     number += k
//     if localarray[k]:
//         localprimelist.append(number);
//         for (j:=k+k, j<=n/b, j += number):
//             localarray[j]=False

pair<vector<int>, vector<int>> localSieve(int s, int b, int N, bulk::world& world) {
    int listlength =  N / b + 1;
    int number = s;
    // if (s != 1) {
    //     number = s;
    // }
    // else {
    //     number = b + 1;
    // }
    
    vector<int> localarray(listlength, 1);
    if (s==1) {
        localarray[0] = 0;
    }

    vector<int> localprimelist;

    for (int k = 0; k < listlength; k ++) {
        if (localarray.at(k)) {
            // world.log("%d", number);
            localprimelist.push_back(number);
            for (int j = k + number; j <= listlength; j += number) {
                localarray[j] = 0;
            }
        }
        number += b;
    }
    // for (int i=0;i <localprimelist.size() ;i++){
    //     world.log("%d", localprimelist.at(i));
    // }
    return make_pair(localarray, localprimelist);
}
    



// distribute_small_numbers(b):
// % input:   A number $b$. 
// % output:  All numbers $s<b$, with $gcd(s,b)=1$, distributed along the processors. 
// processor_number                 := 0      ;
// array(type=bool, b) s_candidates := <True> ;

// for (s:=1, s<b, s+=1):
//     if s_candidates[s]:
//         if b mod s  == 0; 
//             %sieve out all multiples of s by making sure they fail the above check.  
//             for (j:=2s, j<b , j+= s):
//                 s_candidates[j] = False;
//         else:
//             assign(processor_number,s)
//             processor_number +=1 
//             processor_number = processor_number % number_of_procs

// In practice: we do assignment by letting each processor keep its own list of s's, called my_s

pair<vector<int>, vector<int>> b_coprimes(int b, int pid, int p, bulk::world& world) {
    vector<int> s_candidates(b, true);
    vector<int> my_s, s_winners;
    s_winners.push_back(1);
    if (pid == 0) {
        my_s.push_back(1);
    }

    int processor_number = 1;
    // world.log("%d", s_candidates);
    // for (int s = 0; s<b; s++){
    //     world.log("%d", s_candidates.at(s));
    // }

    for (int s = 2; s < b; s ++) {
        // world.log("%d", s_candidates.at(s));
        if (s_candidates.at(s)) {
            if (b % s == 0) {
                for (int j = s + s; j < b; j += s) {
                    s_candidates[j] = false;
                }
            }
            else {
                if (processor_number == pid) {
                    my_s.push_back(s);
                }
                // world.log("\t%d", s);
                s_winners.push_back(s);
                processor_number ++; 
                processor_number = processor_number % p;  
            }
        }
    }

    return make_pair(my_s, s_winners);
}


// \begin{verbatim}
// Calculate_inverses(b,k_list,s)
// input:  A number $b$, 
//         k_list: the list of all numbers k<b with gcd(k,b) =1 (the full s_candidates list for example). 
//         And a number $s$ with gcd(s,b)=1
// output: A dictionary, where the keys consist of the above numbers k, and the value $l$ is such that k*l =s

// inverse_dict_s = dictionary()

// for k in k_list:
//     for v in k_list:
//         if k*v % b == s:
//             inverse_dict_s.store(k,v)
//             inverse_dict_s.store(v,k) 
// skip()


map<int, int> inverse_dict (int b, vector<int> k_list, int s, bulk::world& world) {
    map<int, int> inverse_dict_s;

    for (auto k : k_list) {
        for (auto v : k_list) {
            if ((k * v) % b == s) {
                inverse_dict_s.insert({k, v});
                continue;
            }
        }
    }
    return inverse_dict_s;
}



// \begin{verbatim}
// Calculate_first_multiple(a, s', s, inverse_dict)
// input: a number s' with gcd(s',b)=1
//        a number s  with gcd(s,b) =1
//        a number a with a % b =s'
//        an inverse dictionary as above
// output: a number c with a%b = s, a|c, and the least number with these properties. 

// l = inverse_dict_s(s')
// c=l*a

// \end{verbatim}


void remove_multiples(int s, int b , vector<int>& localarray, int a, int sa, map<int, int> inverse_dict_s, bulk::world& world) {
    // int sa = a mod b, but this is better passed along in application.  
    if (sa == s){
        return;
    }
    int l = inverse_dict_s[sa];
    int first_multiple = l * a;
    int starting_index = (first_multiple - s) / b;
    int listlength = localarray.size();
    for (int i = starting_index; i < listlength; i += a){
        localarray[i] = false;
    }
    
    int number = s; 
    for (int i=0;i <localarray.size() ;i++){
        // world.log("%d, %d, %d", localarray.at(i), s + i * b, a);
    }
}




// \begin{verbatim}
// Calculate_inverses(b,k_list,s)
// input:  A number $b$, 
//         k_list: the list of all numbers k<b with gcd(k,b) =1 (the full s_candidates list for example). 
//         And a number $s$ with gcd(s,b)=1
// output: A dictionary, where the keys consist of the above numbers k, and the value $l$ is such that k*l =s

// inverse_dict_s = dictionary()

// for k in k_list:
//     for v in k_list:
//         if k*v % b == s:
//             inverse_dict_s.store(k,v)
//             inverse_dict_s.store(v,k) 
//             skip()
// \end{verbatim}


// \begin{verbatim}
// Calculate_first_multiple(a, s', s, inverse_dict)
// input: a number s' with gcd(s',b)=1
//        a number s  with gcd(s,b) =1
//        a number a with a % b =s'
//        an inverse dictionary as above
// output: a number c with a%b = s, a|c, and the least number with these properties. 

// l = inverse_dict_s(s')
// c=l*a

// \end{verbatim}



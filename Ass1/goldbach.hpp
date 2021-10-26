vector<size_t> distribute_evens(size_t b, size_t pid) {
    vector<size_t> my_ts; 
    int processor_number = 0; 
    int t = 0;
    for (size_t t = 0; t<b; t+= 2){
        if (processor_number == pid){
            my_ts.push_back(t);
        }
    } 
    return my_ts;
}



// k_list should have both all primes \leq b, and all coprime numbers with b. 

map<size_t, vector< pair<size_t,size_t> > > sum_dicts (size_t b, vector<size_t> k_list, size_t &flops, bulk::world &world) {

    map<size_t, vector<pair<size_t,size_t>> > dict;
    for (auto k : k_list) {
        // world.log("%d",k);
        for (auto v : k_list) {
                if (v>=k){
                    size_t t = (k+v) % b ;
                    dict[t].push_back(make_pair(k,v));
                    flops += 3;
                }
            }
        }
    return dict;
}


bool goldbach_holds(size_t t, size_t b, size_t n, vector< pair<size_t,size_t>> s_summers, map<size_t, vector <size_t>  > true_indices, bulk::world &world){
    size_t list_len = n/b;
    vector<int> goldbach_bools (list_len, 0);  
    for (auto pair: s_summers){
        size_t s1 = pair.first;
        size_t s2 = pair.second;
        // world.log("%d,%d,%d", t,s1,s2);
        if (s1+s2 == t){
            for (auto k1:true_indices[s1]){
                for (auto k2:true_indices[s2]){
                    if(k1+k2<list_len){
                    goldbach_bools[k1+k2] = 1;} 
                }
            }
        }
        else {
            for (auto k1:true_indices[s1]){
                for (auto k2:true_indices[s2]){
                    if(k1+k2+1<list_len){
                    goldbach_bools[k1+k2+1] = 1 ;} 
                }
            }
        }
    }
    int number = t;
    if (t == 2 || t == 0){
        goldbach_bools[0]  = 1;
    }
    for (auto g : goldbach_bools){
        if (! g){
            // world.log("%d", number);
            return false;
        }
        number += b;
    }
    return true;
}
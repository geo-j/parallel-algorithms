#include <vector>


pair<vector<pair<size_t, size_t>>, vector<size_t>> twin_distribute_s(size_t p, size_t pid,  size_t b, vector< size_t> coprimes_b, bulk::world &world) {
    //Assuming 6|b
    vector<size_t> my_s;
    vector<pair<size_t, size_t>> my_twins;

    if (pid == 0) {
        my_s.push_back(1);
        my_s.push_back(b - 1);
        my_twins.push_back(make_pair(1, b - 1));
    }
    
    size_t processor_number = 1;
    size_t index = 1;
    
    while(index < coprimes_b.size() - 1) {
        int s = coprimes_b.at(index);
        if (pid == processor_number){
            my_s.push_back(s);
        }
        // if (index < coprimes_b.size() -2){
        if (coprimes_b.at(index + 1) == s + 2) {
            if (pid == processor_number){
                my_s.push_back(s + 2);
                my_twins.push_back(make_pair(s, s + 2));
            }
            index ++;
        }
        // }
        index ++;
        processor_number ++;
        processor_number = processor_number % p ; 
    }
    return make_pair(my_twins, my_s); 
}

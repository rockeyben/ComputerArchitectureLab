#include "stdio.h"
#include "cache.h"
#include "memory.h"
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <sstream>

using namespace std;
int main(int argc, char * argv[]) {

    ifstream fin;
    int cache_size; // how many 32KBs
    int block_size; // how many Bytes
    int associativity;
    int write_through;
    int write_allocate;
    int freq = 2;

    CacheConfig l1_config;
    string request;
    string s_req_addr;


    // read trace mode
    if(!strcmp(argv[1],"-t"))
    {
        fin.open(argv[2]);
        string s_cs = string(argv[3]);
        string s_bs = string(argv[4]);
        string ass = string(argv[5]);
        string s_wt = string(argv[6]);
        string s_wa = string(argv[7]);

        // set params
        stringstream ss;
        ss << s_cs;
        ss >> cache_size;
        ss.clear();
        ss << s_bs;
        ss >> block_size;
        ss.clear();
        ss << ass;
        ss >> associativity;
        ss.clear();
        ss << s_wt;
        ss >> write_through;
        ss.clear();
        ss << s_wa;
        ss >> write_allocate;
        l1_config.size = cache_size;
        l1_config.associativity = associativity;
        l1_config.set_num = cache_size * 32 * 1024 / block_size / associativity;
        l1_config.write_through = write_through;
        l1_config.write_allocate = write_allocate;
        l1_config.block_size = block_size;

    }


    Memory m;
    Cache l1;
    l1.SetLower(&m);
    l1.SetConfig(l1_config);

    StorageStats s;
    s.access_time = 0;
    s.access_counter = 0;
    m.SetStats(s);
    l1.SetStats(s);

    StorageLatency ml;
    ml.bus_latency = 0;
    ml.hit_latency = 100;
    m.SetLatency(ml);

    StorageLatency ll;
    ll.bus_latency = 0;
    ll.hit_latency = 10;
    l1.SetLatency(ll);

    int total_hit = 0;
    while(fin >> request >> s_req_addr)
    {
        //cout<<request<<" "<<s_req_addr<<endl;
        int hit, time;
        char content[64];
        /*
        l1.HandleRequest(0, 0, 1, content, hit, time);
        printf("Request access time: %dns\n", time);
        l1.HandleRequest(1024, 0, 1, content, hit, time);
        printf("Request access time: %dns\n", time);*/

        uint64_t req_addr;
        stringstream ss;
        ss << s_req_addr;
        ss >> req_addr;
        if(request == "r")
        {
            l1.HandleRequest(req_addr, 1, 1, content, hit, time);
        }
        else if(request == "w")
        {
            l1.HandleRequest(req_addr, 1, 0, content, hit, time);
        }
        total_hit += hit;
    }

    l1.GetStats(s);
    printf("Total L1 access time: %dns\n", s.access_time / freq);
    printf("Total l1 access: %d\n", s.access_counter);
    m.GetStats(s);
    printf("Total Memory access time: %dns\n", s.access_time / freq);
    printf("Total hit: %d\n", total_hit);
    return 0;
}

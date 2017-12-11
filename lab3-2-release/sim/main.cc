#include "stdio.h"
#include "cache.h"
#include "memory.h"
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <sstream>
#include <map>

using namespace std;

map<uint64_t, int> USED_bit;
map<uint64_t, int> bypassBuf; 
int NRU_count = 0;
int NRU_period = 100;

int main(int argc, char * argv[]) {

    ifstream fin;
    int cache_size; // how many 32KBs
    int block_size; // how many Bytes
    int associativity;
    int write_through;
    int write_allocate;

    CacheConfig l1_config;
    CacheConfig l2_config;
    string request;
    string s_req_addr;

    int prefetch_1 = 0;
    int prefetch_2 = 0;

    int bypass_1 = 0;
    int bypass_2 = 0;

    int replace_1 = 0;
    int replace_2 = 0;

    if(argc < 2)
    {
        cout << "wrong arg format" << endl;
        cout << "please read the README.md" << endl;
        cout << "use -h or --help to know how to use" << endl;
        return 0;
    }

    // read trace mode
    for(int i =0; i < argc; i++)
    {
        if(!strcmp(argv[i],"--prefetch"))
        {
            prefetch_1 = prefetch_2 = 1;
        }
        else if(!strcmp(argv[i],"--bypass"))
        {
            bypass_2 = 1;
        }
        else if(!strcmp(argv[i], "--replace-mode"))
        {
            if(!strcmp(argv[i+1],"0"))
            {
                replace_1 = replace_2 = 0;
            }
            else if(!strcmp(argv[i+1],"1"))
            {
                replace_1 = replace_2 = 1;
            }
            else if(!strcmp(argv[i+1],"2"))
            {
                replace_1 = 2;
                replace_2 = 2;
            }
        }
        else if((!strcmp(argv[i], "-h"))||(!strcmp(argv[i],"--help")))
        {
            cout << "if you want to simulate *.trace, please type in:" << endl;
            cout << "  ./sim *.trace " << endl;
            cout << "[OPTIONS]:"<<endl;
            cout << "  --prefetch      if you want to start prefetch mode, and both 2 cache will use prefetch" << endl;
            cout << "  --bypass        if you want to start bypass mode, and only l2 cache will use bypass" << endl;
            cout << "  --replace-mode  : 0 - LRU: Least Recently Used (default)" << endl;
            cout << "                    1 - NRU: Not Recently Used" << endl;
            cout << "                    2 - Tree-PLRU: Binary Tree Pseudo LRU" << endl;
            return 0;
        }
    }





    l1_config.id = 1;
    l1_config.size = 32;
    l1_config.block_size = 64;
    l1_config.associativity = 8;
    l1_config.write_through = 0;
    l1_config.set_num = l1_config.size * 1024 / 64 / 8;
    l1_config.use_prefetch = prefetch_1;
    l1_config.use_bypass = bypass_1;
    l1_config.replace_mode = replace_1;

    l2_config.id = 2;
    l2_config.size = 256;
    l2_config.block_size = 64;
    l2_config.associativity = 8;
    l2_config.write_through = 0;
    l2_config.set_num = l2_config.size * 1024 / 64 / 8;
    l2_config.use_prefetch = prefetch_2;
    l2_config.use_bypass = bypass_2;
    l2_config.replace_mode = replace_2;

    Memory m;
    Cache l1;
    Cache l2;

    l1.SetLower(&l2);
    l1.SetConfig(l1_config);
    l2.SetLower(&m);
    l2.SetConfig(l2_config);

    StorageStats s;
    s.access_time = 0;
    s.access_counter = 0;
    s.miss_num = 0;
    m.SetStats(s);
    l1.SetStats(s);
    l2.SetStats(s);

    StorageLatency ml;
    ml.bus_latency = 0;
    ml.hit_latency = 100;
    m.SetLatency(ml);

    StorageLatency ll;
    ll.bus_latency = 0;
    ll.hit_latency = 3;
    l1.SetLatency(ll);

    StorageLatency l2l;
    l2l.bus_latency = 6;
    l2l.hit_latency = 4;
    l2.SetLatency(l2l);

    int miss1_penalty = ll.bus_latency + l2l.hit_latency;
    int miss2_penalty = l2l.bus_latency + ml.hit_latency;

    int total_hit = 0;
    int cnt = 0;

    for(int i = 0; i < 10; i++)
    {
         fin.open(argv[1]);

        if(!fin.is_open())
        {
            cout << "failed to open this file, please check if existed" << endl;
            return 0;
        }
        while(fin >> request >> s_req_addr)
        {
            NRU_count ++;
            //cout<<s_req_addr<<endl;
            int hit, time;
            char content[64];

            uint64_t req_addr;
            stringstream ss;
            ss <<hex<< s_req_addr;
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
        fin.close();
    }
    l1.GetStats(s);
    int l1_access = s.access_counter;
    int l1_access_time = s.access_time;
    int l1_miss = s.miss_num;
    printf("Total L1 access time: %d cycles\n", s.access_time);
    printf("Total l1 access: %d\n", s.access_counter);
    printf("Total l1 miss: %d\n", s.miss_num);
    l2.GetStats(s);
    int l2_access = s.access_counter;
    int l2_access_time = s.access_time;
    int l2_miss = s.miss_num;
    printf("Total L2 access time: %d cycles\n", s.access_time);
    printf("Total l2 access: %d\n", s.access_counter);
    printf("Total l2 miss: %d\n", s.miss_num);
    m.GetStats(s);
    printf("Total Memory access time: %d cycles\n", s.access_time);
    printf("Total Memory access: %d cycles\n", s.access_counter);
    printf("l1 miss rate: %f\n", (float)l1_miss/l1_access);
    printf("l2 miss rate: %f\n", (float)l2_miss/l2_access);
    float miss1 = (float)l1_miss/(float)l1_access;
    float miss2 = (float)l2_miss/(float)l2_access;
    StorageLatency lll;
    l2.GetLatency(lll);

    /*@author

        AMAT = l1.hit_latency + miss1 * (l1.bus_latency + l2.hit_latency) + miss1 * miss2 * (l2.bus_latency + m.hit_latency)
    
    */
    printf("AMAT: %f\n", 3 + miss1*miss2*miss2_penalty+miss1*miss1_penalty);

    
    return 0;
}
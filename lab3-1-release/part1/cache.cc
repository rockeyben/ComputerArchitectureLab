#include "cache.h"
#include "def.h"
#include <memory.h>

void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
    hit = 0;
    time = 0;
    stats_.access_counter ++;

    int rep;
    uint64_t block_index = getbit(addr, 0, config_.b_len - 1);
    uint64_t set_index = getbit(addr, config_.b_len, config_.b_len + config_.idx_len - 1);
    uint64_t tag = getbit(addr, 64 - config_.tag_len, 63);
    //cout<<config_.b_len<<" "<<config_.tag_len<<" "<<config_.idx_len<<" "<<config_.set_num<< endl;
    //cout<<hex<<"index: "<<set_index<<" tag: "<<tag<<endl;
    // Bypass?
    if (!BypassDecision()) {
        PartitionAlgorithm();
        // read
        if(read == 1)
        {
            // Miss
            if (ReplaceDecision(block_index, set_index, tag) == FALSE) {
                // Choose victim
                rep = ReplaceAlgorithm(block_index, set_index, tag);
            }
            // hit 
            else {

                for(int j = 0; j < config_.associativity; j++)
                {
                    if(set[set_index].info[j].valid == 1 && set[set_index].info[j].tag == tag)
                    {
                        rep = j;
                        break;
                    }
                }

                // return hit & time
                //cout<<"cache read hit"<<endl;
                hit = 1;
                time += latency_.bus_latency + latency_.hit_latency;
                stats_.access_time += time;
                for(int i = 0; i < bytes; i++)
                {
                    content[i] = set[set_index].data[rep].block[block_index + i];
                }
                return;
            }
        }
        // write
        else
        {
            // write through
            if(config_.write_through == 1)
            {
                // write allocate
                if(config_.write_allocate == 1)
                {
                    // Miss
                    if (ReplaceDecision(block_index, set_index, tag) == FALSE) {
                        // Choose victim
                        rep = ReplaceAlgorithm(block_index, set_index, tag);
                    }
                    else
                    {
                        hit = 1;
                        for(int j = 0; j < config_.associativity; j++)
                        {
                            if(set[set_index].info[j].valid == 1 && set[set_index].info[j].tag == tag)
                            {
                                rep = j;
                                break;
                            }
                        }
                    }

                    for(int i = 0; i < bytes; i++)
                        set[set_index].data[rep].block[i + block_index] = content[i];
                }
            }
            // write back
            else
            {
                // Miss
                if (ReplaceDecision(block_index, set_index, tag) == FALSE) {
                    // Choose victim
                    rep = ReplaceAlgorithm(block_index, set_index, tag);
                }
                else
                {
                    hit = 1;
                    for(int j = 0; j < config_.associativity; j++)
                    {
                        if(set[set_index].info[j].valid == 1 && set[set_index].info[j].tag == tag)
                        {
                            rep = j;
                            break;
                        }
                    }
                }

                // if this block haven't been changed, then temporally store the content in the cache
                if(set[set_index].info[rep].dirty == 0)
                {
                    set[set_index].info[rep].dirty = 1;
                    //hit = 1;
                    time += latency_.bus_latency + latency_.hit_latency;
                    stats_.access_time += time;
                    for(int i = 0; i < bytes; i++)
                        set[set_index].data[rep].block[i + block_index] = content[i];
                    return ;
                }
            }
        }
    }
    // Prefetch?
    if (PrefetchDecision()) {
        PrefetchAlgorithm();
    } else {
        // Fetch from lower layer
        int lower_hit, lower_time;
        lower_->HandleRequest(addr, bytes, read, content,
                          lower_hit, lower_time);
        //hit = 0;
        time += latency_.bus_latency + lower_time;
        stats_.access_time += latency_.bus_latency;

        if(read == 1 || (read == 0 && config_.write_through == 0))
        {
            for(int i = 0; i < bytes; i++)
            {
                set[set_index].data[rep].block[block_index + i] = content[i];
            }
        }
    }


    // update LRU situation
    for(int i = 0; i < config_.set_num; i++)
    {
        for(int j = 0; j < config_.associativity; j++)
        {
            if(set[i].info[j].valid == 1)
            {
                set[i].info[j].use_time ++;
            }
        }
    }

}

int Cache::BypassDecision() {
  return FALSE;
}

void Cache::PartitionAlgorithm() {
}

// Decide whether this request is missed
int Cache::ReplaceDecision(uint64_t block_index,uint64_t set_index,uint64_t tag) {
    for(int j = 0; j < config_.associativity; j++)
    {
        if(set[set_index].info[j].valid == 1 && set[set_index].info[j].tag == tag)
        {
            return TRUE;
        }
    }
    return FALSE;
}

int Cache::ReplaceAlgorithm(uint64_t block_index, uint64_t set_index, uint64_t tag){
    int LRU_idx = 0;
    int LRU_num = set[set_index].info[0].use_time;
    for(int j = 0;j < config_.associativity; j++)
    {
        // this block is not full, so choose an empty one
        if(set[set_index].info[j].valid == 0)
        {
            set[set_index].info[j].valid = 1;
            set[set_index].info[j].tag = tag;
            set[set_index].info[j].use_time = 0;
            return j;
        }
        else{
            if(set[set_index].info[j].use_time > LRU_num)
            {
                LRU_idx = j;
                LRU_num = set[set_index].info[j].use_time;
            }
        }
    }

    set[set_index].info[LRU_idx].valid = 1;
    set[set_index].info[LRU_idx].tag = tag;
    set[set_index].info[LRU_idx].use_time = 0;
    return LRU_idx;
}

int Cache::PrefetchDecision() {
  return FALSE;
}

void Cache::PrefetchAlgorithm() {
}

void Cache::SetConfig(CacheConfig cc)
{
    cc.b_len = log2(cc.block_size);
    cc.idx_len = log2(cc.set_num);
    cc.tag_len = 64 - cc.b_len - cc.idx_len;
    config_ = cc;
    set = (CacheSet*)malloc(config_.set_num*sizeof(CacheSet));
    for(int i = 0; i < config_.set_num; i++)
    {
        set[i].data = (Data*)malloc(config_.associativity*sizeof(Data));
        for(int j = 0; j < config_.associativity; j++)
            set[i].data[j].block = (char*)malloc(config_.block_size*sizeof(char));
        set[i].info = (Info*)malloc(config_.associativity*sizeof(Info));
    }
}

uint64_t getbit(uint64_t inst,int s,int e)
{
    uint64_t mask1 = 0xffffffffffffffff;
    uint64_t mask2 = 0xffffffffffffffff;
    uint64_t one = 1;
    if(s > 0)
    {
        mask1 =  ~((one << (s-1)) - 1);
    }

    if(e < 63)
    {
        mask2 =  (one << (e+1)) - 1;
    }
    if(e == 64)
    {
        //printf("get op : %x %x %x %x\n",inst, inst & mask1 & mask2, mask1, mask2);
    }
    uint64_t res = inst & mask1 & mask2;
    res = res >> s;
    return res;
}

int log2(int val)
{
    int res = 0;
    while(val > 1)
    {
        res ++;
        val = val >> 1;
    }


    return res;
}
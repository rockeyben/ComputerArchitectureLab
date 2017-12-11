#include "cache.h"
#include "def.h"
#include <memory.h>

extern map<uint64_t, int> USED_bit;
extern map<uint64_t, int> bypassBuf; 
extern int NRU_count;
extern int NRU_period;

void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
    //cout<<"cache "<<config_.id<<" is handling this request"<<endl;

    hit = 0;
    time = 0;
    stats_.access_counter ++;

    uint64_t block_index = getbit(addr, 0, config_.b_len - 1);
    uint64_t set_index = getbit(addr, config_.b_len, config_.b_len + config_.idx_len - 1);
    uint64_t tag = getbit(addr, 64 - config_.tag_len, 63);
    //cout<<config_.b_len<<" "<<config_.tag_len<<" "<<config_.idx_len<<" "<<config_.set_num<< endl;
    //cout<<hex<<"index: "<<set_index<<" tag: "<<tag<<endl;
    
    // Bypass?

    int rep=ReplaceDecision(addr, set_index, tag);
    int missed = 0;

    uint64_t block_addr = (tag << (config_.b_len + config_.idx_len)) | (set_index << config_.b_len);

    if (config_.use_bypass >= 1) 
    {  
        uint64_t one = 1;
        uint64_t B_addr = addr & (~(one << (config_.b_len) -1));
        // if we use bypass mode, we need to update our USED_bit and bypass buffer
        if(USED_bit.count(B_addr) == 0)
        {
            USED_bit.insert(pair<uint64_t, int>(B_addr, 0));
        }

        if(bypassBuf.count(B_addr) == 0)
        {
            bypassBuf.insert(pair<uint64_t, int>(B_addr, 0));
        }
        if(rep == FALSE)
        {
            stats_.miss_num ++;
            missed = 1;
            // if miss , then we talk about something of bypass
            if(BypassDecision(B_addr, set_index, tag, config_.use_bypass)==TRUE)
            {
                //cout<<"bypass"<<endl;
                // do bypass
                int lower_hit, lower_time;
                lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
                time += latency_.bus_latency + lower_time;
                stats_.access_time += latency_.bus_latency;
                return ;
            }
            // allocate cache for this block
            else{
                //cout << "happen" <<endl;
                // Not Recently Used - NRU algorithm
                // choose a victim block
                rep = ReplaceAlgorithm(block_addr, set_index, tag);
                uint64_t victim_addr = (set[set_index].info[rep].tag << (config_.b_len+config_.idx_len)) | (set_index << config_.b_len);
                victim_addr = victim_addr & (~(one << (config_.b_len+2)-1));
                bypassBuf[victim_addr] = USED_bit[victim_addr]*3;
                USED_bit[B_addr] = 0;
            }
        }
        // if hit, we only need to update USED_bit and leave the rest work to the following code.
        else
        {
            USED_bit[B_addr] = 1;
        }
    }


    // read
    if(read == 1)
    {
        // Miss
        if (rep == FALSE || missed == 1) {
            // Choose victim
            if(config_.use_bypass == 0)
            {
                rep = ReplaceAlgorithm(addr, set_index, tag);
                stats_.miss_num ++;
            }  
            missed = 1;
        }
        // hit 
        else {

            rep = rep - 1;
            // return hit & time
            hit = 1;
            time += latency_.bus_latency + latency_.hit_latency;
            stats_.access_time += time;
            for(int i = 0; i < bytes; i++)
            {
                content[i] = set[set_index].data[rep].block[i+block_index];
            }
            return;
        }
    }
    // write
    else
    {
        // Miss
        if (rep == FALSE || missed == 1) {
            // Choose victim
            if(config_.use_bypass == 0)
            {
                stats_.miss_num ++;
                rep = ReplaceAlgorithm(addr, set_index, tag);
            }
            missed = 1;
        }
        else
        {
            hit = 1;
            rep = rep - 1;

            // if this block haven't been changed, then temporally store the content in the cache
            set[set_index].info[rep].dirty = 1;
            time += latency_.bus_latency + latency_.hit_latency;
            stats_.access_time += time;
            for(int i = 0; i < bytes; i++)
                set[set_index].data[rep].block[i+block_index] = content[i];
            return ;
        }
    }


    // Prefetch, fetch data into stream buffer
    if(missed == 1 && config_.use_prefetch == 1)
    {
        int rep_block = PrefetchHit(block_addr);
        if(rep_block > 0)
        {
            time += latency_.bus_latency;
            stats_.access_time += latency_.bus_latency;
            stats_.miss_num --;

            // insert the hit block from stream buffer into cache
            
            rep = ReplaceAlgorithm(block_addr, set_index, tag);

            int lower_hit, lower_time;

            if(set[set_index].info[rep].dirty == 1 && set[set_index].info[rep].valid == 1)
            {
                lower_->HandleRequest(block_addr, 64, 0, set[set_index].data[rep].block, lower_hit, lower_time);
                time += lower_time;
            }

            time += latency_.hit_latency;
            
            set[set_index].info[rep].tag = tag;
            set[set_index].info[rep].valid = 1;
            
            return ;
        }

        int pre_idx = PrefetchDecision(block_addr);
        if(pre_idx > 0)
        {
            PrefetchAlgorithm(pre_idx - 1);
        }
    }

    // Fetch from lower layer
    int lower_hit, lower_time;
    char * tmp_content = (char*)malloc(config_.block_size*sizeof(char));
    memcpy(tmp_content, set[set_index].data[rep].block, config_.block_size*sizeof(char));

    /*@author

        Four conditions

        Main implemetation of the Write Back Strategy

    */

    if(read == 0 && config_.write_through == 0 && missed == 1)
    {
        if(set[set_index].info[rep].dirty == 1)
        {
            uint64_t victim_addr = (set_index << config_.b_len) | (set[set_index].info[rep].tag << (config_.b_len + config_.idx_len));
            lower_->HandleRequest(victim_addr, 64, 0, set[set_index].data[rep].block, lower_hit, lower_time);
        }

        lower_->HandleRequest(block_addr, config_.block_size, 1, tmp_content, lower_hit, lower_time);
        memcpy(tmp_content+block_index, content, bytes*sizeof(char));
        memcpy(set[set_index].data[rep].block, tmp_content, config_.block_size*sizeof(char));
        set[set_index].info[rep].dirty = 1;
    }
    else if(read == 0 && config_.write_through == 0 && missed == 0)
    {
        lower_->HandleRequest(block_addr, config_.block_size, 0, tmp_content, lower_hit, lower_time);
        memcpy(set[set_index].data[rep].block+block_index, content, bytes*sizeof(char));
    }
    else if(read == 1 && config_.write_through == 0)
    {
        if(set[set_index].info[rep].dirty == 1)
        {
            uint64_t victim_addr = (set_index << config_.b_len) | (set[set_index].info[rep].tag<< (config_.b_len + config_.idx_len));
            lower_->HandleRequest(victim_addr, 64, 0, set[set_index].data[rep].block, lower_hit, lower_time);
            set[set_index].info[rep].dirty = 0;
        }

        lower_->HandleRequest(block_addr, config_.block_size, read, tmp_content, lower_hit, lower_time);
        memcpy(set[set_index].data[rep].block, tmp_content, config_.block_size*sizeof(char));
        memcpy(content, set[set_index].data[rep].block+block_index, bytes*sizeof(char));
    }
    else
    {
        lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
    }

    set[set_index].info[rep].tag = tag;
    time += latency_.bus_latency + lower_time;
    stats_.access_time += latency_.bus_latency;

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


int Cache::BypassDecision(uint64_t addr, uint64_t set_index, uint64_t tag, int mode){

    // Mode 1. CBTs based on Reuse-Count
    // implementation of Kharbutli's work
    if(mode == 1)
    {
        if(bypassBuf[addr] < 3)
        {
            bypassBuf[addr]++;
            return TRUE;
        }

        return FALSE;
    }

    return FALSE;
}

int Cache::ReplaceAlgorithm(uint64_t addr, uint64_t set_index, uint64_t tag){

    int rep = 0;

    // Tree-PLRU
    if(config_.replace_mode == 2)
    {
        int tmp_node = 0;
        for(int j = 0; j < log2(config_.associativity); j++)
        {
            // go to left child node
            if(set[set_index].tree[tmp_node] == 0)
            {
                tmp_node = tmp_node * 2;
                set[set_index].tree[tmp_node] = 1;
            }
            else
            {
                tmp_node = tmp_node * 2 + 1;
                set[set_index].tree[tmp_node] = 0;
            }
        }

        rep = tmp_node;
        set[set_index].info[rep].valid = 1;
        return rep;
    }

    // if find an empty block
    for(int j = 0; j < config_.associativity; j++)
    {
        if(set[set_index].info[j].valid == 0)
        {
            rep = j;
            set[set_index].info[j].valid = 1;

            // LRU
            if(config_.replace_mode == 0)
            {
                set[set_index].info[j].use_time = 0;
            }
            // NRU
            else if(config_.replace_mode == 1)
            {
                set[set_index].info[j].NRU_use = 1;
                for(int k = 0; k < config_.associativity; k++)
                {
                    if(set[set_index].info[k].valid == 1 && set[set_index].info[k].NRU_use == 1 && k!=j)
                        set[set_index].info[k].NRU_use = 0;
                }
            }
            return rep;
        }
    }


    // if the cache is full

    // LRU
    if(config_.replace_mode == 0)
    {
        int LRU_idx = 0;
        int LRU_num = set[set_index].info[0].use_time;
    
        for(int j = 0;j < config_.associativity; j++)
        {
            if(set[set_index].info[j].use_time > LRU_num)
            {
                LRU_idx = j;
                LRU_num = set[set_index].info[j].use_time;
            }
        }

        set[set_index].info[LRU_idx].valid = 1;
        set[set_index].info[LRU_idx].use_time = 0;
        return LRU_idx;
    }
    // NRU
    else if(config_.replace_mode == 1)
    {
        for(int j = 0; j < config_.associativity; j++)
        {
            uint64_t tmp_tag = set[set_index].info[j].tag;
            uint64_t block_addr = (tag << (config_.b_len+config_.idx_len)) | (set_index << config_.b_len);
            if(set[set_index].info[j].NRU_use == 0)
            {
                rep = j;
                set[set_index].info[j].NRU_use = 1;
                break;
            }
        }

        for(int j = 0; j < config_.associativity; j++)
        {
            if(j != rep)
            {
                set[set_index].info[j].NRU_use = 0;
            }
        }

        if(NRU_count == NRU_period)
        {
            for(int j = 0; j < config_.associativity; j++)
            {
                if(j != rep && set[set_index].info[j].NRU_use == 1)
                    set[set_index].info[j].NRU_use = 0;
            }
            NRU_count = 0;
        }

        return rep;
    }

}

// Decide whether this request is missed
int Cache::ReplaceDecision(uint64_t addr, uint64_t set_index,uint64_t tag) {
    for(int j = 0; j < config_.associativity; j++)
    {
        if(set[set_index].info[j].valid == 1 && set[set_index].info[j].tag == tag)
        {
            return j+1;
        }
    }
    return FALSE;
}

int Cache::PartitionAlgorithm(uint64_t addr, uint64_t set_index, uint64_t tag)
{

}

int Cache::PrefetchHit(uint64_t addr)
{
    for(int i = 0; i < stream_size; i++)
    {
        if(stream[i].valid == 1&& addr == stream[i].addr)
        {
            /*@author
                Only compare with the head of stream.
                Stream is a FIFO queue.
                The hit stream buffer should be loaded into cache, since we use the write back strategy.
                For further notice, we don't fetch new block after one block is loaded into cache, since
            the Lab limits that we can only fetch when the miss happens, but now is the 'hit' condition.
            In the paper, the author does fetch new block after one is invalidated.
            */
            if(stream[i].block_valid > 0)
            {
                stream[i].block_valid --;
                // update queue head
                stream[i].addr += config_.block_size;
                if(stream[i].block_valid == 0)
                    stream[i].valid = 0;
                return TRUE; 
            }
            else
            {
                return FALSE;
            }

        }  
    }

    return FALSE;
}

int Cache::PrefetchDecision(uint64_t addr) {

    // find an empty stream to allocate
    /*@author
        Example: when cache miss address A, then we put A+1 in the stream buffer
        There is no need to put A into the stream buffer, because A will be allocated in the
        cache if the miss happens.
    */

    for(int i =0; i < stream_size; i++)
    { 
        if(stream[i].empty == 1)
        {
            stream[i].valid = 1;
            stream[i].addr = addr + config_.block_size;
            stream[i].empty = 0; 
            stream[i].block_valid = 4;
            return FALSE;
        }
    }

    // If no empty steam, allocate to an invalid one
    for(int i = 0; i < stream_size; i++)
    {
        if(stream[i].valid == 0)
        {
            stream[i].valid = 1;
            stream[i].addr = addr + config_.block_size;
            stream[i].empty = 0;
            stream[i].block_valid = 4;
            return FALSE;
        }
    }

    // Finally, if no invalid steam, use evict algorithm FIFO(first in first out) to replace one stream
    FIFO += 1;
    if(FIFO == stream_size)
    {
        FIFO = 0;
    }

    stream[FIFO].addr = addr+config_.block_size;
    stream[FIFO].empty = 0;
    stream[FIFO].valid = 1;
    stream[FIFO].block_valid = 4;

    return FALSE;
}

void Cache::PrefetchAlgorithm(int idx) {
    /*
    fetch from memory, since the requirements said the prefetching doesn't cost time
    so we skip this part for correctness.
    */
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
        set[i].tree = (int*)malloc((config_.associativity)*sizeof(int));
        for(int j = 0; j < config_.associativity; j++)
            set[i].tree[j] = 0;
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
        /*
            do nothing
        */
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

void Cache::ClearDirty()
{
    for(int i = 0; i < config_.set_num;i++)
    {
        for(int j = 0; j < config_.associativity; j++)
        {
            if(set[i].info[j].valid == 1 && set[i].info[j].dirty == 1)
            {
                uint64_t idx = (uint64_t)i;
                uint64_t tag = set[i].info[j].tag;
                uint64_t dirty_addr = (idx << config_.b_len) | (tag << (config_.b_len + config_.idx_len));
                int hit, time;
                lower_->HandleRequest(dirty_addr, config_.block_size, 0, set[i].data[j].block, hit, time);
            }
        }
    }
}
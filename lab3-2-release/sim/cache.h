#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <map>


using namespace std;

uint64_t getbit(uint64_t inst,int s,int e);
int log2(int val);

typedef struct CacheConfig_ {
	int id;
	int size;
	int associativity;
	int set_num; // Number of cache sets
	int block_size; // size of block
	int write_through; // 0|1 for back|through
	int write_allocate; // 0|1 for no-alc|alc
	int b_len;
	int tag_len;
	int idx_len;
	int use_prefetch;
	int use_bypass;
	int replace_mode;
	CacheConfig_() {replace_mode = 0; write_through = 0; use_prefetch = 0; use_bypass = 0;}
} CacheConfig;

typedef struct Data_
{
	char * block;
	Data_() {block = NULL;}
} Data;

typedef struct Info_
{
	uint64_t tag;
	int valid;
	int use_time;
	int dirty;
	int NRU_use;
	Info_() {tag = 0; valid = 0; use_time = 0;dirty = 0; NRU_use = 0;}
} Info;

typedef struct Set_
{
	Data * data;
	Info * info;
	int * tree;
	Set_() {data = NULL; info = NULL; tree = NULL;}
} CacheSet;

typedef struct Stream_
{
	uint64_t addr;
	int valid;
	int empty;
	int stride;
	int size;
	int block_valid;
	Stream_(){valid = 0;stride = 1; empty = 1; block_valid=4;}
} Stream;

class Cache: public Storage {
 public:
 	int stream_size;
	Cache() {set = NULL;
		FIFO = 0;
		stream_size = 8;
		stream = (Stream*)malloc(stream_size*sizeof(Stream));
		for(int i = 0; i < stream_size; i++)
		{
			stream[i].valid = 0;
			stream[i].empty = 1;
			stream[i].addr = 0;
			stream[i].size = 64*4*8;
			stream[i].block_valid = 4;
		}
	}
	~Cache() {}

	CacheSet * set;

	Stream * stream;
	int FIFO;

	// Sets & Gets
	void SetConfig(CacheConfig cc);
	void GetConfig(CacheConfig &cc){cc = config_;}
	void SetLower(Storage *ll) { lower_ = ll; }
	// Main access process
	void HandleRequest(uint64_t addr, int bytes, int read,
										 char *content, int &hit, int &time);
	void ClearDirty();

 private:
	// Bypassing
	int BypassDecision(uint64_t addr, uint64_t set_index, uint64_t tag, int mode);
	// Partitioning
	int PartitionAlgorithm(uint64_t addr, uint64_t set_index, uint64_t tag);
	// Replacement
	int ReplaceDecision(uint64_t addr, uint64_t set_index,uint64_t tag);
	int ReplaceAlgorithm(uint64_t addr, uint64_t set_index, uint64_t tag);
	// Prefetching
	int PrefetchDecision(uint64_t addr);
	void PrefetchAlgorithm(int s_idx);
	int PrefetchHit(uint64_t addr);
	void InsertBlock(uint64_t addr, uint64_t set_index, uint64_t tag, char* content);

	CacheConfig config_;
	Storage *lower_;
	DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 


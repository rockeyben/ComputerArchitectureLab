#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"
#include <stdlib.h>
#include <iostream>

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
	Info_() {tag = 0; valid = 0; use_time = 0;dirty = 0;}
} Info;

typedef struct Set_
{
	Data * data;
	Info * info;
	Set_() {data = NULL; info = NULL;}
} CacheSet;



class Cache: public Storage {
 public:
	Cache() {set = NULL;}
	~Cache() {}

	CacheSet * set;


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
	int BypassDecision();
	// Partitioning
	void PartitionAlgorithm();
	// Replacement
	int ReplaceDecision(uint64_t block_index,uint64_t set_index,uint64_t tag);
	int ReplaceAlgorithm(uint64_t block_index, uint64_t set_index, uint64_t tag);
	// Prefetching
	int PrefetchDecision();
	void PrefetchAlgorithm();

	CacheConfig config_;
	Storage *lower_;
	DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 

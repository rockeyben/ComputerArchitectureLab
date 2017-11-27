#include "memory.h"

void Memory::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
	hit = 1;
	time = latency_.hit_latency + latency_.bus_latency;
	stats_.access_time += time;
	//cout<<"bytes:"<<bytes<<endl;
	/*
	if(read == 1)
	{
		for(int i = 0; i < bytes; i++)
		{
			cout<<"OK"<<endl;
			content[i] = memory[addr + i];
		}
		cout<<"OK"<<endl;
	}
	else
	{
		for(int i = 0; i < bytes; i++)
		{
			memory[addr + i] = content[i];
		}
	}*/
}


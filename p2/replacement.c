#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "disk.h"

int frameCount = 0;
int lfuArray[MAX_FRAME];
int lruArray[MAX_FRAME];

int random_algo()
{
	int next = rand() % MAX_PAGE;
	return next;
}

int fifo()
{
	int next = frameCount % MAX_PAGE;
	frameCount++;

	return next;	
}

void insert_lru(int frameNo) {
	lruArray[frameNo] = timeStamp;
}

int lru()
{
	int minFrame = 0;
	int minTimeStamp = lruArray[minFrame];
	int result = 0;

	for (int i = 0; i < MAX_FRAME; ++i)
	{
		if (minTimeStamp > lruArray[i]) {
			minFrame = i;
			minTimeStamp = lruArray[i];
		}
	}
	result = minFrame;

	lruArray[minFrame] = timeStamp;
	return result;
}

void insert_lfu(int frameNo) {
	lfuArray[frameNo]++;
}

int lfu()
{
	int min = 0;
	int minCount = lfuArray[min];
	for (int i = 0; i < MAX_FRAME; ++i)
	{
		if (lfuArray[i] < minCount)
		{
			min = i;
			minCount = lfuArray[min];
		}
	}

	lfuArray[min] = 1;

	return min;
}

int page_replacement()
{
		int frame;
		if(replacementPolicy == RANDOM)  frame = random_algo(); 
		else if(replacementPolicy == FIFO)  frame = fifo();
		else if(replacementPolicy == LRU) frame = lru();
		else if(replacementPolicy == LFU) frame = lfu();

		return frame;
}


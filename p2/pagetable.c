#include <stdio.h>
#include "vm.h"
#include "disk.h"
#include "pagetable.h"

PT pageTable;
Invert invert_table[MAX_FRAME];
int frameList = 0;

int hit_test(int pid, int pageNo, char type)
{
 	if (pageTable.entry[pid][pageNo].valid == true) {
 		int currentFrame = pageTable.entry[pid][pageNo].frameNo;
 		if (invert_table[currentFrame].pid == pid && invert_table[currentFrame].page == pageNo) {
 			pageTable.stats.hitCount++;
 			insert_lfu(currentFrame);
 			insert_lru(currentFrame);
 			if (type == 'W')
 			{
 				pageTable.entry[pid][pageNo].dirty = true;
 			}

 			return pageTable.entry[pid][pageNo].frameNo;
 		}
 	}
	return -1;
}

int pagefault_handler(int pid, int pageNo, char type, bool *hit)
{
	//frame not full
	pageTable.stats.missCount++;
	*hit = false;

	if (frameList < MAX_PAGE) {
		//不换下
		//换上
		int temp = frameList;
		insert_lfu(temp);
		insert_lru(temp);
		pageTable.entry[pid][pageNo].frameNo = temp;

		invert_table[temp].pid = pid;
		invert_table[temp].page = pageNo;

		pageTable.entry[pid][pageNo].valid = true;
		disk_read(temp, pid, pageNo);
		frameList++;
		return temp;
	}
	else {	
		int frameNo = page_replacement();
		//frame full
		//换下frameNo xianzai de dongxi
		//1)kanxubuxuyao write
		//2) valid = false
		if (type == 'W')
		{
			disk_write(frameNo, pid, pageNo);
		}
		pageTable.entry[pid][pageNo].valid = false;
		
		invert_table[frameNo].pid = pid;
		invert_table[frameNo].page = pageNo;

		pageTable.entry[pid][pageNo].frameNo = frameNo;
		pageTable.entry[pid][pageNo].valid = true;
		disk_read(frameNo, pid, pageNo);
		//huanshang FRAME
		return frameNo;
	}
}

int MMU(int pid, int addr, char type, bool *hit)
{
		int frameNo;
		int pageNo = (addr >> 8);
		int offset = addr - (pageNo << 8);
		
		if(pageNo > MAX_PAGE) {
				printf("invalid page number (MAX_PAGE = 0x%x): pid %d, addr %x\n", MAX_PAGE, pid, addr);
				return -1;
		}
		if(pid > MAX_PID) {
				printf("invalid pid (MAX_PID = %d): pid %d, addr %x\n", MAX_PID, pid, addr);
				return -1;
		}

		// hit
		frameNo = hit_test(pid, pageNo, type);
		if(frameNo > -1) {
				*hit = true;
				return (frameNo << 8) + offset;
		}
		
		// pagefault
		frameNo = pagefault_handler(pid, pageNo, type, hit);
		return (frameNo << 8) + offset;
}

void pt_print_stats()
{
		int req = pageTable.stats.hitCount + pageTable.stats.missCount;
		int hit = pageTable.stats.hitCount;
		int miss = pageTable.stats.missCount;

		printf("Request: %d\n", req);
		printf("Hit: %d (%.2f%%)\n", hit, (float) hit*100 / (float)req);
		printf("Miss: %d (%.2f%%)\n", miss, (float)miss*100 / (float)req);
}

void pt_init()
{
		int i,j;

		pageTable.stats.hitCount = 0;
		pageTable.stats.missCount = 0;
		for(i = 0; i < MAX_PID; i++) {
				for(j = 0; j < MAX_PAGE; j++) {
						pageTable.entry[i][j].valid = false;
				}
		}
}


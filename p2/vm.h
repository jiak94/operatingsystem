#include <stdbool.h>

#define NDEBUG

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG(%s:%d) " M, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define RANDOM 0
#define FIFO 1
#define LRU 2
#define LFU 3

#define MAX_PAGE 8
#define MAX_FRAME 3
#define MAX_PID 1

extern int replacementPolicy;
extern int timeStamp;

int page_replacement();
int MMU(int pid, int addr, char type, bool *hit);
void pt_print_stats();
void pt_init();
void disk_print_stats();
void insert_lfu(int frameNo);
void insert_lru(int frameNo);

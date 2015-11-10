typedef struct {
		int pid;
		int page;
} Invert;

typedef struct {
		int frameNo;
		bool valid;
		bool dirty;
} PTE;

typedef struct {
		int hitCount;
		int missCount;
} STATS;

typedef struct {
		PTE entry[MAX_PID][MAX_PAGE];
		STATS stats;
} PT;

int pagefault_handler(int pid, int pageNo, char type, bool *hit);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#define PSIZE 	256
#define PMAX	256
#define FSIZE 	256
#define FMAX	128
#define TMAX	16	
#define SWAP_LRU	1
#define SWAP_FIFO	0	


typedef struct _TLB {	// TLB
	int pnum;			// page number
	int fnum;			// frame number
	clock_t count;		// LRU_TLB를 위해 사용, 참조될 때마다 갱신
} TLB;
typedef struct _Ptab {	// Page table
	int fnum;			// frame number
	int valid;			// valid bit
} Ptab;
typedef struct _Ftab {	// Frame table
	int pnum;			// page number
	int valid;			// valid bit
	clock_t count;		// LRU를 위해 사용, 참조될 때마다 갱신
	clock_t start;		// FIFO을 위해 사용, 처음 frame에 page를 넣을 때 갱신
} Ftab;

void initAll();						// 사용 되는 변수들을 초기화

int searchTLB(int p);				// TLB를 탐색하기 위해 사용
void insertTLB(int p, int fnum);	// TLB에 값을 넣기 위해 사용
void LRU_TLB(int p, int fnum);		// TLB가 가득 찼을 때 victim을 선택하기 위해 사용

int searchPtab(int p);				// Page table을 탐색하기 위해 사용
void insertPtab(int p, int check);	// Page table에 entry를 넣기 위해 사용
void LRU(int p);					// 모든 frame이 사용중일 때, LRU로 할당할 곳을 찾기 위해 사용
void FIFO(int p);					// 모든 frame이 사용중일 때, FIFO로 할당할 곳을 찾기 위해 사용

void manageBS(int pnum, int fnum);	// Backing store를 관리하기 위한 함수
void printHit(int check);			// 각각의 hit rate을 출력하기 위한 함수

void openFile(char* argv[], int check);			// 필요한 파일들을 open
void closeFile();					// open한 파일들을 close
void makeSnap(int check);			// Physical memory와 frame table의 snapshot을 파일로 만들기 위한 함수

Ptab ptab[PMAX];					// Page table
Ftab ftab[FMAX];					// Frame table
TLB tlb[TMAX];						// TLB

char physical_memory[FMAX*FSIZE];	// Physical memory의 크기만큼 배열 할당

int ladr;							// Logical address를 받아옴  
int p, d;							// Page number, offset
int total, fault, fault_tlb;		// Hit ratio 계산 위함
FILE* re;							// File의 read 위한 변수 
FILE* wr;							// File의 write 위한 변수
FILE* _wr;							// File의 write 위한 변수
FILE* bs;							// Swap in/out시 backing store를 읽어 오기 위한 변수

int main (int argc, char* argv[]) {
	///////////////////////////
	//TLB와 LRU를 이용한 방법//
	///////////////////////////
	initAll();								// 필요한 변수들 초기화
	openFile(argv, SWAP_LRU);						// 필요한 file들 open
	while(1) {
		fscanf(re, "%d", &ladr);			// Addresses.txt에서 logical address를 읽어옴
		if(feof(re))						// File의 끝일 때 
			break;							// 반복문 나감
		p = ladr / PSIZE;					// Logical address의 앞부분을 page number로	
		d = ladr % PSIZE;					// 뒷부분을 offset으로
		++total;							// Hit ratio를 위한 전체 개수 누적

		if(!searchTLB(p)) {					// TLB에 해당 page number가 없을 시
			++fault_tlb;					// TLB의 fault 증가
			if(!searchPtab(p)) {			// Page table에 해당 page number가 없을 시
				++fault;					// Page table의 fault를 증가
				insertPtab(p, SWAP_LRU);	// P해당 page number를 LRU 방식으로 swap in
				searchPtab(p);				// falut였기 때문에 다시 검색
			}
		}	
	}
	closeFile();							// 열린 file들 close
	printHit(SWAP_LRU);						// Hit ratio 출력
	makeSnap(SWAP_LRU);						// Snapshot 저장
	
	////////////////////////
	//FIFO만을 이용한 방법//
	////////////////////////
	initAll();								// 필요한 변수들 초기화
	openFile(argv, SWAP_FIFO);					// 필요한 file들 open
	while (1) {
		fscanf(re, "%d", &ladr);			// Addresses.txt에서 logical address를 읽어옴
		if (feof(re))						// File의 끝일 때 
			break;							// 반복문 나감
		p = ladr / PSIZE;					// Logical address의 앞부분을 page number로	
		d = ladr % PSIZE;					// 뒷부분을 offset으로
		++total;							// Hit ratio를 위한 전체 개수 누적

		if (!searchPtab(p)) {				// Page table에 해당 page number가 없을 시
			++fault;						// fault를 증가
			insertPtab(p, SWAP_FIFO);		// page number를 FIFO 방식으로 swap in
			searchPtab(p);					// falut였기 때문에 다시 검색
		}
	}
	closeFile();
	makeSnap(SWAP_FIFO);					// Snapshot 저장
	printHit(SWAP_FIFO);					// Hit ratio 출력

	return 0;
}
void initAll() {
	int i;
	total = 0;
	fault = 0;
	fault_tlb = 0;
	memset(tlb, -1, sizeof(tlb));		// TLB를 -1로 전부 초기화
	memset(ptab, -1, sizeof(ptab));		// Page table을 -1로 전부 초기화
	memset(ftab, -1, sizeof(ftab));		// Frame table을 -1로 전부 초기화
	for(i=0; i<FMAX; ++i)				// Frame table의 valid bit를 전부 0으로 초기화
		ftab[i].valid = 0;
	for(i=0; i<PMAX; ++i)				// Frame table의 valid bit를 전부 0으로 초기화
		ptab[i].valid = 0;
}
void openFile(char* argv[], int check) {
	if(check) {
		re = fopen(argv[1], "r");					// Logical address 가져올 파일
		wr = fopen("Physical_LRU.txt", "w");		// Logical address를 physical address 바꾼 값 저장할 파일
		bs = fopen("BACKING_STORE.bin", "rb");		// Backing store 관리 위한 파일
		_wr = fopen("Physical_detail_LRU.txt", "w");// Logical address를 physical address 바꾼 값 상세히 나타낸 파일
	}
	else {
		re = fopen(argv[1], "r");					// Logical address 가져올 파일
		wr = fopen("Physical_FIFO.txt", "w");		// Logical address를 physical address 바꾼 값 저장할 파일
		bs = fopen("BACKING_STORE.bin", "rb");		// Backing store 관리 위한 파일
		_wr = fopen("Physical_detail_FIFO.txt", "w");// Logical address를 physical address 바꾼 값 상세히 나타낸 파일
	}
	if(re==NULL) {
		printf("re fail\n");
		exit(0);
	}
	if(wr==NULL) {
		printf("wr fail\n");
		exit(0);
	}
	if(bs==NULL) {
		printf("bs fail\n");
		exit(0);
	}
	if(_wr==NULL) {
		printf("_wr fail\n");
		exit(0);
	}
}
void closeFile() {
	fclose(re);
	fclose(wr);
	fclose(bs);
	fclose(_wr);
}
void makeSnap(int check) {
	int i;
	if(check) {
		wr = fopen ("Frame_table_LRU.txt", "w");			// 현재 frame table의 상태를 저장
		_wr = fopen("Physical_memory_LRU.bin", "wb");		// 현재 physical memory의 상태를 저장
	}
	else {
		wr = fopen ("Frame_table_FIFO.txt", "w");			// 현재 frame table의 상태를 저장
		_wr = fopen("Physical_memory_FIFO.bin", "wb");		// 현재 physical memory의 상태를 저장
	}
	for(i=0; i<FMAX; ++i) {
		fprintf(wr, "%d %d %d\n", i, ftab[i].valid, 
				ftab[i].pnum * FSIZE);
	}
	for(i=0; i<FMAX*FSIZE; ++i) {
		fprintf(_wr,"%c",physical_memory[i]);
	}
	fclose(wr);
	fclose(_wr);
}
int searchTLB(int p) {								// page number를 받아서 TLB에 있는지 확인
	int i;
	for(i=0; i<TMAX; ++i) {							// TLB entry 전체를 탐색
		if(tlb[i].pnum == p) {						// TLB에 해당하는 page가 있으면
			int fnum = tlb[i].fnum;					// 해당 entry의 frame number 확인

			tlb[i].count = clock();					// LRU_TLB 위해서 해당 entry의 접근 시간 갱신
			ftab[fnum].count = tlb[i].count;		// LRU 위해서 frame table의 entry 값 갱신

			fprintf(wr, "%d\n", fnum * FSIZE + d);	// Physical.txt에 Physical address 값 저장
			fprintf(_wr, "Virtual address: %d Physical address: %d\n",ladr, fnum * FSIZE + d);	// Physical.txt에 Physical address 값 저장
			return fnum;							// frame number return
		}
	}
	return 0;										// Hit 안됐을 시 0 return
}
void insertTLB(int p, int fnum) {
	int i;
	for(i=0; i<TMAX; ++i) {
		if(tlb[i].count == -1) {				// 사용이 안된 entry가 존재시
			tlb[i].pnum = p;					// 해당 entry에 page number
			tlb[i].fnum = fnum;					// frame number
			tlb[i].count = ftab[fnum].count;	// 접근 시간 갱신
			return;								// 종료
		}	
	}
	LRU_TLB(p, fnum);							// 비어있는 entry를 찾지 못했을 시
}
void LRU_TLB(int p, int fnum) {
	int i;
	int idx;
	clock_t cur = clock();						// 현재 시각 저장

	float diff;
	float mx = 0;
	for (i = 0; i<TMAX; ++i) {
		diff = (float)(cur - tlb[i].count);		// 현재 시각과 해당 frame table의 차 저장
		if (mx < diff) {						// 가장 차이가 큰 값의 정보를 저장
			mx = diff;		
			idx = i;
		}
	}
	tlb[idx].pnum = p;							// 차이가 가장 큰 위치에 page number와
	tlb[idx].fnum = fnum;						// frame number 저장
	tlb[idx].count = ftab[fnum].count;			// clock() 갱신
}
int searchPtab(int p) {
	if(ptab[p].valid) {							// Page table에서 해당 page가 할당된 frame이 있을 시
		int fnum = ptab[p].fnum;				// 해당 frame number를 받고
		ftab[fnum].count = clock();				// frame table에서 해당 frame의 count 값 갱신
		insertTLB(p, fnum);						// TLB에서 hit가 안됐기 때문에 TLB 갱신
		fprintf(wr, "%d\n", fnum * FSIZE + d);	// Physical.txt에 Physical address 값 저장
		fprintf(_wr, "Virtual address: %d Physical address: %d\n", ladr, fnum * FSIZE + d);	// Physical.txt에 Physical address 값 저장
		return 1;								// hit 되었으므로 return 1
	}
	return 0;									// miss 되었으므로 return 0
}
void insertPtab(int p, int check) {				// Page table에 해당 page가 할당된 frame이 없을 시
	int i;
	for(i=0; i<FMAX; ++i) {						// 모든 frame을 찾아서
		if(!ftab[i].valid) {					// 할당되지 않은 frame이 있을 시
			int fnum;
			fnum = i;							
			ftab[fnum].pnum = p;				// frame table의 해당 frame number에 page number 갱신
			if (check)
				manageBS(p, fnum);				// Swap in 되었기 때문에 Backing store에서 해당 page 가져옴
			ftab[i].valid = 1;					// frame table의 frame number에 해당하는 valid bit를 1로 갱신
			ftab[i].start = clock();			// FIFO를 위해서 시작시간 저장

			ptab[p].fnum = fnum;				// 해당 page number에 frame number 부여
			ptab[p].valid = 1;					// page table의 page number에 해당하는 valid bit를 1로 갱신
			return;
		} 
	}
	if(check)
		LRU(p);									// 비어있는 page가 없을 시 LRU로 victim 선택
	else
		FIFO(p);								// 비어있는 page가 없을 시 FIFO로 victim 선택
}
void LRU(int p) {
	int i;
	int fnum;
	clock_t cur = clock();						// 현재 시각을 저장

	float diff;
	float mx = 0;
	int pprev;
	for(i=0; i<FMAX; ++i) {
		diff = (float)(cur - ftab[i].count);	// 현재 시각과 frame의 count 차가
		if(mx < diff) {							// 가장 큰 값의 정보를 저장
			mx = diff;
			fnum = i;	
		}
	}
	pprev = ftab[fnum].pnum;					// 차이가 가장 큰 frame의 page number에 해당하는 entry
	ptab[pprev].valid = 0;						// page table에서 찾아서 valid bit를 0으로 갱신

	manageBS(p, fnum);							// Swap 되었기 때문에 Backing store에서 해당 page 가져옴
	ftab[fnum].pnum = p;						// frame table에 해당 page를 저장하고

	ptab[p].fnum = fnum;						// page table도 갱신
	ptab[p].valid = 1;							// 해당 page number에 해당하는 valid bit 1로 갱신
}
void FIFO(int p) {
	int i;
	int fnum = 0;
	clock_t cur = ftab[0].start;				

	int pprev;
	for(i=0; i<FMAX; ++i) {						
		clock_t next = ftab[i].start;
		if(cur > next) {						// 제일 처음에 할당한 frame의 위치 저장
			fnum = i;	
			cur = next;
		}
	}
	pprev = ftab[fnum].pnum;					// 저장한 frame의 page number에 해당하는
	ptab[pprev].valid = 0;						// entry의 valid bit을 0으로 갱신

	ftab[fnum].pnum = p;						// frame table에 해당 page를 저장하고
	ftab[fnum].start = clock();					// 할당한 시각 갱신

	ptab[p].fnum = fnum;						// page table도 갱신
	ptab[p].valid = 1;							// page number에 해당하는 valid bit 1로 갱신
}
void manageBS(int pnum, int fnum) {
	int	i;
	char buf[FSIZE];
	fseek(bs, pnum*PSIZE, SEEK_SET);				// 해당 page의 위치를 찾아서
	fread(buf, 1, FSIZE, bs);						// 읽어서
	for(i=0; i<FSIZE; ++i) {
		physical_memory[fnum*FSIZE + i] = buf[i];	// Physical memory에 저장
	}
}
void printHit(int check) {					
	if(check) {
		printf("TLB hit ratio : %d hits out of %d\n", total-fault_tlb, total);
		printf("LRU hit ratio : %d hits out of %d\n", total-fault, total);
	}
	else
		printf("FIFO hit ratio : %d hits out of %d\n", total-fault, total);
}

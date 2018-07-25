#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "buffer.h"

buffer_item buffer[BUFFER_SIZE];
pthread_mutex_t mtx;						// consumer와 producer의 critical section에 접근할 때를 위함
pthread_mutex_t mtx_mon;					// monitoring 진입위한 mutex
pthread_mutex_t mtx_proc;					// producer monitoring의 block을 위함
pthread_mutex_t mtx_cons;					// consumer monitoring의 block을 위함
int monitoring_item;						// monitor에서 확인을 위한 공유변수
int cnt;									// item의 개수 파악

int insert_item(buffer_item item) {			// buffer에 item을 넣기 위함
	if (cnt < BUFFER_SIZE) {				// 현재 item의 개수가 BUFFER_SIZE보다 작다면
		buffer[cnt++] = item;				// buffer에 item을 추가
		return 0;							// 정상종료
	}
	else {									// buffer에 item을 담을 공간이 없으면
		return -1;							// 비정상종료
	}
}
int remove_item(buffer_item *item) {		// buffer에서 item을 빼기 위함
	if (cnt > 0) {							// buffer에 item이 하나 이상 있을 시
		int i;
		*item = buffer[0];					// FIFO 형태로 item 가져옴
		for (i = 1; i < cnt; ++i)			// 첫번째 것을 가져 갔으므로 배열 밀어줌
			buffer[i - 1] = buffer[i];
		--cnt;								// item 개수 하나 줄임
		return 0;							// 정상종료
	}
	else {									// buffer에 item이 없을 시
		return -1;							// 비정상종료
	}
}
void *monitorProc() {									// Producer에서 생산한 item이 1~50 사이의 값인지 확인 
	printf("----Producer monitoring start----\n");
	while (1) {
		pthread_mutex_lock(&mtx_proc);					// item을 확인하지 않을 때는 block
		int item = monitoring_item;						// item을 받아서
		printf("-------monitor-------\n");
		if (1 <= item&&item <= 50) {					// 값이 1~50사이 일 때
			printf("| Item : %d, grant  |\n", item);	// grant
			printf("---------------------\n");
			monitoring_item = 0;						// monitoring의 결과 grant임을 알리기 위해 0 넣음
		}
		else {											// 값이 범위 밖일 때
			printf("| Item : %d, reject |\n", item);	// reject
			printf("---------------------\n");
			monitoring_item = -1;						// monitoring의 결과 reject임을 알리기 위해 -1 넣음
		}
		pthread_mutex_unlock(&mtx_mon);					// producer thread 다시 동작 시켜주기 위해 사용
	}
}
void *monitorCons() {									// Consumer에서 받은 item이 1~25 사이의 값인지 확인
	printf("----Consumer monitoring start----\n");
	while (1) {
		pthread_mutex_lock(&mtx_cons);					// item을 확인하지 않을 때는 block
		int item = monitoring_item;						// item을 받아서
		printf("-------monitor-------\n");
		if (1 <= item&&item <= 25) {					// 값이 1~25사이 일 때
			printf("| Item : %d, grant  |\n", item);	// grant
			printf("---------------------\n");
			monitoring_item = item;						// grant한 item 값을 다시 보내줌
		}
		else {											// 값이 범위 밖일 때
			printf("| Item : %d, divide |\n", item);	// divide
			printf("---------------------\n");
			monitoring_item = item / 2;					// divide한 item 값을 다시 보내줌
		}
		pthread_mutex_unlock(&mtx_mon);					// consumer thread 다시 동작 시켜주기 위해 사용
	}
}
void state() {									// 현재 buffer의 contents 값 확인
	int i;
	printf("Contents : ");
	if (cnt == 0) {								// buffer에 item이 없을 때
		printf("empty\n");						// empty 출력
	}
	else if (cnt == BUFFER_SIZE) {					// buffer가 가득 찼을 때
		printf("full\n");						// full 출력
	}
	else {										// 정상적인 상태일 때
		printf("|");
		for (i = 0; i<cnt; ++i)					// buffer의 값들을 하나하나 보여줌
			printf("%d|", buffer[i]);
		puts("");
	}
}
void *producer(void *param) {						// producer함수
	buffer_item item;								// item 생성 위함
	int idx = (int*)param;							// 몇번 producer인지 확인 위함
	int ret;										// monitoring결과 받기 위함
	while (1) {
		sleep(rand() % 5);							// 0~4초 sleep
		item = rand() % 100;						// 0~99값 생산

		pthread_mutex_lock(&mtx);					// 다른 producer나 consumer가 buffer에 진입하지 못하게 하기 위함
		printf("\n\n===========Producer-%d\n", idx);// 몇번 producer인지 확인
		monitoring_item = item;						// monitoring을 통해 현재 생산한 item을 확인 받기 위함
		pthread_mutex_unlock(&mtx_proc);			// block해 놓은 monitoring thread의 동작을 위함
		pthread_mutex_lock(&mtx_mon);				// 현재 producer thread를 block

		if (monitoring_item) {						// monitoring 결과 reject
			state();								// buffer상태 보여줌
			pthread_mutex_unlock(&mtx);				// mutex_unlock
			continue;								// 다시 생산하기 위해 continue
		}
		else if (insert_item(item))					// 값이 정상적이나 buffer가 가득 찼을 때
			printf("report error condition\n");		// error 출력
		else										// 정상적일 때
			printf("producer produced %d\n", item);	// 생산한 item 출력
		state();									// buffer상태 보여줌
		pthread_mutex_unlock(&mtx);					// 다른 producer나 consumer가 buffer에 진입하도록 해줌
	}
}
void *consumer(void *param) {							// consumer 함수
	buffer_item item;									// item받기 위함
	int idx = (int*)param;								// 몇번 째 consumer인지 확인 위함
	while (1) {
		sleep(rand() % 5);								// 0~4초 간 sleep

		pthread_mutex_lock(&mtx);						// 다른 producer나 consumer가 buffer에 진입하지 못하게 하기 위함
		printf("\n\n===========Consumer-%d\n", idx);	// 몇번 consumer인지 출력
		if (remove_item(&item))							// buffer가 empty일 시
			printf("report error condition\n");			// 에러 출력
		else {											// buffer가 empty가 아니라면
			monitoring_item = item;						// monitoring을 통해 해당 item을 검사받음
			pthread_mutex_unlock(&mtx_cons);			// monitoring동안 consumer thread를 block하기 위함
			pthread_mutex_lock(&mtx_mon);				// block된 monitor를 동작시키기 위함

			printf("consumer consumed %d\n", 
				monitoring_item);						// monitoring 결과 나타난 item 소비
		}
		state();										// buffer 상태 출력
		pthread_mutex_unlock(&mtx);						// 다른 producer나 consumer가 buffer에 진입하도록 해줌
														
	}
}
int main(int argc, char *argv[]) {
	/* 1. Get command line arguments argv[1], argv[2], argv[3] */
	int sleepTime = atoi(argv[1]);					// sleep time
	int numOfProd = atoi(argv[2]);					// producer 개수
	int numOfCons = atoi(argv[3]);					// consumer 개수
	int i, j;
	pthread_t prod[numOfProd], cons[numOfCons];		// producer와 consumer thread 생성 위함
	pthread_t monitor_proc;							// producer monitoring thread 생성 위함
	pthread_t monitor_cons;							// consumer monitoring thread 생성 위함
	/* 2. Initialize buffer */
	memset(buffer, -1, sizeof(buffer));				// buffer 초기화
	pthread_mutex_init(&mtx, NULL);					// mtx초기화
	pthread_mutex_init(&mtx_mon, NULL);				// mtx_mon 초기화
	pthread_mutex_init(&mtx_proc, NULL);			// mtx_proc 초기화
	pthread_mutex_init(&mtx_cons, NULL);			// mtx_cons 초기화
	
	pthread_mutex_lock(&mtx_proc);					// 처음부터 block을 
	pthread_mutex_lock(&mtx_cons);					// 하기 위하여 
	pthread_mutex_lock(&mtx_mon);					// 미리 mutex 값을 줄여 놓음
	sleep(1);										// thread 생성 전에 특정 mutex 값들을 확실하게 줄여 놓기 위함

	/* Create monitoring thread(s) */
	pthread_create(&monitor_proc, NULL, monitorProc,
		NULL);										// producer monitoring 실행
	sleep(1);										// producer monitoring thread를 먼저 생성하기 위함
	pthread_create(&monitor_cons, NULL, monitorCons,
		NULL);										// consumer monitoring 
	sleep(1);										// producer와 consumer thread 이전에 생성하는 것을 확실시 하기 위함

	/* 3. Create producer thread(s) */
	for (i = 0; i<numOfProd; ++i)						// produces 개수만큼 thread 생성
		pthread_create(&prod[i], NULL, producer, (void*)i);
	/* 4. Create consumer thread(s) */
	for (i = 0; i<numOfCons; ++i)						// consumer 개수만큼 thread 생성
		pthread_create(&cons[i], NULL, consumer, (void*)i);
	/* 5. Sleep */
	sleep(sleepTime);									// thread들 실행동안 sleep
	/* 6. Exit */
	pthread_mutex_destroy(&mtx);						// 생성했던
	pthread_mutex_destroy(&mtx_mon);					// mutex들을
	pthread_mutex_destroy(&mtx_proc);					// 전부
	pthread_mutex_destroy(&mtx_cons);					// destroy
	printf("\n===Program finished===\n");
	return 0;
}



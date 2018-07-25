#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "buffer.h"

buffer_item buffer[BUFFER_SIZE];
pthread_mutex_t mtx;						// critical section에 접근할 때를 위함
//sem_t empty, full;						// empty일 때 full일 때 block되기 위함이었으나, 사용하지 않음
int cnt;									// item의 개수 파악

int insert_item(buffer_item item) {			// buffer에 item을 넣기 위함
	if(cnt < BUFFER_SIZE) {					// 현재 item의 개수가 BUFFER_SIZE보다 작다면
		buffer[cnt++] = item;				// buffer에 item을 추가
		return 0;							// 정상종료
	}
	else{									// buffer에 item을 담을 공간이 없으면
		return -1;							// 비정상종료
	}
}
int remove_item(buffer_item *item) {		// buffer에서 item을 빼기 위함
	if(cnt > 0) {							// buffer에 item이 하나 이상 있을 시
		int i;
		//*item = buffer[cnt-1];				
		//--cnt;
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
void *monitorProc(void *param) {				// Producer에서 생산한 item이 1~50 사이의 값인지 확인 
	int item=(int*)param;						// item을 받아서
	printf("----Start monitor----\n");
	if(1<=item&&item<=50) {						// 값이 1~50사이 일 때
		printf("| Item : %d, grant  |\n", item);// grant
		printf("---------------------\n");
		pthread_exit((void*)0);					// 정상종료
	}
	else {										// 값이 범위 밖일 때
		printf("| Item : %d, reject |\n", item);// reject
		printf("---------------------\n");
		pthread_exit((void*)-1);				// 비정상종료
	}
}
void *monitorCons(void *param) {				// Consumer에서 받은 item이 1~25 사이의 값인지 확인
	int item=(int*)param;						// item을 받아서
	printf("----Start monitor----\n");
	if(1<=item&&item<=25) {						// 값이 1~25사이 일 때
		printf("| Item : %d, grant  |\n", item);// grant
		printf("---------------------\n");
		pthread_exit((void*)item);				// item 반환
	}
	else {										// 값이 범위 밖일 때
		printf("| Item : %d, divide |\n", item);// divide
		printf("---------------------\n");
		item/=2;
		pthread_exit((void*)item);				// item/2 반환
	}
}
void state() {									// 현재 buffer의 contents 값 확인
	int i;
	printf("Contents : ");				
	if(cnt==0) {								// buffer에 item이 없을 때
		printf("empty\n");						// empty 출력
	}
	else if(cnt==BUFFER_SIZE) {					// buffer가 가득 찼을 때
		printf("full\n");						// full 출력
	}
	else {										// 정상적인 상태일 때
		printf("|");
		for(i=0; i<cnt; ++i)					// buffer의 값들을 하나하나 보여줌
			printf("%d|", buffer[i]);
		puts("");
	}
}
void *producer(void *param) {						// producer함수
	pthread_t monitor;								// monitoring thread 생성 위해 선언
	buffer_item item;								// item 생성 위함
	int idx = (int*)param;							// 몇번 producer인지 확인 위함
	int ret;										// monitoring결과 받기 위함
	while(1) {
		sleep(rand()%5);							// 0~4초 sleep
		item = rand()%100;							// 0~99값 생산

		//sem_wait(&empty);
		pthread_mutex_lock(&mtx);					// critical section 진입 위해 semaphore값 하나 줄임
		printf("\n\n===========Producer-%d\n", idx);	// 몇번 producer인지 확인
		pthread_create(&monitor, NULL, monitorProc, 
				(void*)item);						// monitoring 실행
		pthread_join(monitor, (void**)&ret);		// monitoring 결과 ret에 저장
		if(ret){									// monitoring 결과 reject
			state();								// buffer상태 보여줌
			pthread_mutex_unlock(&mtx);				// mutex_unlock
			//sem_post(&empty);	
			continue;								// 다시 생산하기 위해 continue
		}
		else if(insert_item(item))					// 값이 정상적이나 buffer가 가득 찼을 때
			printf("report error condition\n");		// error 출력
		else										// 정상적일 때
			printf("producer produced %d\n", item);	// 생산한 item 출력
		state();									// buffer상태 보여줌
		pthread_mutex_unlock(&mtx);					// semaphore값 하나 증가
		//sem_post(&full);
	}
}
void *consumer(void *param) {							// consumer 함수
	buffer_item item;									// item받기 위함
	pthread_t monitor;									// monitoring thread 생성 위해 선언
	int idx = (int*)param;								// 몇번 째 consumer인지 확인 위함
	while(1) {	
		sleep(rand()%5);								// 0~4초 간 sleep
		
		//sem_wait(&full);
		pthread_mutex_lock(&mtx);						// critical section 진입위해 semaphore값 1 감소
		printf("\n\n===========Consumer-%d\n", idx);	// 몇번 consumer인지 출력
		if(remove_item(&item))							// buffer가 empty일 시
			printf("report error condition\n");			// 에러 출력
		else {											// buffer가 empty가 아니라면
			pthread_create(&monitor, NULL, monitorCons, 
					(void*)item);						// item이 조건에 맞는지 monitoring
			pthread_join(monitor, (void**)&item);		// 조건에 맞는 item값이 된것을 받고
			printf("consumer consumed %d\n", item);		// 해당 item 출력
		}
		state();										// buffer 상태 출력
		pthread_mutex_unlock(&mtx);						// semaphore값 1 증가
		//sem_post(&empty);
	}
}
int main(int argc, char *argv[]) {
	/* 1. Get command line arguments argv[1], argv[2], argv[3] */
	int sleepTime = atoi(argv[1]);					// sleep time
	int numOfProd = atoi(argv[2]);					// producer 개수
	int numOfCons = atoi(argv[3]);					// consumer 개수
	int i, j;
	pthread_t prod[numOfProd], cons[numOfCons];		// thread 생성 위함
	/* 2. Initialize buffer */
	memset(buffer, -1, sizeof(buffer));				// buffer 초기화
	pthread_mutex_init(&mtx, NULL);					// mtx초기화
	//sem_init(&full, 0, 0);						
	//sem_init(&empty, 0, BUFFER_SIZE);			
	/* 3. Create producer thread(s) */
	for(i=0; i<numOfProd; ++i)						// produces 개수만큼 thread 생성
		pthread_create(&prod[i], NULL, producer, (void*)i);
	/* 4. Create consumer thread(s) */
	for(i=0; i<numOfCons; ++i)						// consumer 개수만큼 thread 생성
		pthread_create(&cons[i], NULL, consumer, (void*)i);
	/* 5. Sleep */
	sleep(sleepTime);								// thread들 실행동안 sleep
	/* 6. Exit */
	pthread_mutex_destroy(&mtx);	
	printf("\n===Program finished===\n");
	return 0;	
}



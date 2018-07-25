#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
int matrix[9][9];
int check[10];
int isokay[12] = { 1,1,1,1,1,1,1,1,1,1,1,1 };
sem_t sem;
void* f(void* num) {
	int i, j, k;
	int n = *(int*)num;
	int visit[10];
	switch (n) {
	case 10:
	case 11:	
		for (i = 0; i < 9; ++i) {
			for (j = 0; j < 9; ++j) {
				if (n == 10) {		// 공유 변수인 check에 접근할 때 race condition이 발생하지 않기 위해서 semaphore 사용
					sem_wait(&sem);	// 가로를 조사
					check[matrix[i][j]]++;
					sem_post(&sem);
					visit[matrix[i][j]] = 1;
				}
				else {				// 공유 변수인 check에 접근할 때 race condition이 발생하지 않기 위해서 semaphore 사용
					sem_wait(&sem);	// 세로를 조사
					check[matrix[j][i]]++;
					sem_post(&sem);
					visit[matrix[i][j]] = 1;
				}
			}
			for (k = 1; k <= 9; ++k) {
				if (!visit[k]) {	// 1~9까지 모두 존재하는지 확인
					isokay[n] = 0;
				}
			}
		}
		break;
	default:		 // area가 1~9까지 있고, n이 area 번호 일 때, area의 행 시작을 ((n - 1) / 3) * 3, 열 시작을 ((n - 1) % 3) * 3로 표현
		for (i = ((n - 1) / 3) * 3; i < 3 + ((n - 1) / 3) * 3; ++i) {
			for (j = ((n - 1) % 3) * 3; j < 3 + ((n - 1) % 3) * 3; ++j) {  // 공유 변수인 check에 접근할 때 race condition이 발생하지 않기 위해서 semaphore 사용
				sem_wait(&sem);
				check[matrix[j][i]]++;
				sem_post(&sem);
				visit[matrix[i][j]] = 1;
			}
		}
		for (k = 1; k <= 9; ++k) {
			if (!visit[k])			// 1~9까지 모두 존재하는지 확인
				isokay[n] = 0;
		}
	}
	pthread_exit(NULL);
}
int main() {
	FILE * fp = NULL;
	char c;
	int i = 0, j = 0;
	pthread_t t_id[11];
	int t_val[11];
	int ans;
	int ret = sem_init(&sem, 0, 1);
	if (ret != 0) {
		perror("Semaphore initialization failted");
		return -1;
	}
	fp = fopen("input.txt", "r");

	if (fp != NULL) {						// input.txt에서 한 숫자 씩 받아와서, sudoku를 위한 table을 만드는 과정
		while ((c = getc(fp))!=EOF) {		
			if ('1' <= c && c <= '9') {		// 공백이나 개행을 제거하기 위함
				matrix[i][j] = c - '0';
				++j;
				if (j == 9) {
					j = 0;
					++i;
				}
			}
		}
	}
	else {
		printf("file open fail");			// 파일이 없을 때를 대비한 에러처리
	}
	for (i = 0; i < 9; ++i) {				// 주어진 테이블 출력
		for (j = 0; j < 9; ++j) {
			printf("%d ", matrix[i][j]);
		}
		printf("\n");
	}

	for (i = 0; i < 11; ++i) {				// 각 thread들을 생성하는 구간
		t_val[i]=i+1;					
		pthread_create(&t_id[i], NULL, f,(void*)&t_val[i]);
	}
	for (i = 0; i < 11; ++i) {				// 각 thread들이 끝나기를 기다리는 구간
		pthread_join(t_id[i], NULL);
	}

	ans = 1;
	for (i = 1; i <= 11; ++i) {
		if (isokay[i]) {
			printf("Thread %d : True", i);	// i번 thread의 검사 결과가 참일 때
			if (i <= 9)
				printf(", %d is visited : %d times\n", i, check[i]);	// 각 번호 방문 횟수 출력
			else
				printf("\n");
		}
		else {
			ans = 0;
			printf("Thread %d : False", i);	// // i번 thread의 검사 결과가 거짓일 때
			if (i <= 9)
				printf(", %d is visited : %d times\n", i, check[i]);	// 각 번호 방문 횟수 출력
			else
				printf("\n");
		}
	}

	if(ans)	{
		printf("Valid result !\n");
	}
	else
		printf("Invalid result !\n");
	sem_destroy(&sem);
	return 0;
}


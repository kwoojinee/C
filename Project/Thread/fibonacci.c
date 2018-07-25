#include <stdio.h>
#include <pthread.h>
#include <string.h>
#define MX_SIZE 20
long long arr[MX_SIZE];
void* fibonacci(void* num) {
	int n = *(int *)num;	
	int n1 = n - 1;
	int n2 = n - 2;
	pthread_t t_id;

	if (n <= 1) 
		arr[n] = n;
	else {
		pthread_create(&t_id, NULL, fibonacci, (void*)&n1);		// n-1한 값으로 thread를 생성
		fibonacci((void*)&n2);									// 병행수행을 위해 pthread_join 이전에 n-2한 함수를 호출
		pthread_join(t_id, NULL);
		arr[n] = arr[n - 1] + arr[n - 2];						// 앞서 저장한 배열 값을 통해 index n의 값 저장
	}
}
int f(int n) {
	if (n <= 1)
		return arr[n] = n;
	arr[n - 1] = f(n - 1);		// n-1번 째의 fibonacci 수열 값
	arr[n - 2] = f(n - 2);		// n-2번 째의 fibonacci 수열 값
	return arr[n-1] + arr[n-2];
}
int main() {
	int n, i;
	pthread_t t_id;
	clock_t startTime, endTime;
	double nProcessExcuteTime;
	//////////////////////////////////	thread를 생성하여 계산하였을 시
	printf("\nplease input under 20 : ");
	scanf("%d", &n);
	--n;	// 0 base로 작성하기 때문에 하나를 줄임
	printf("\n-----Create thread\n");
	startTime = clock();	// 현재 시각
	pthread_create(&t_id, NULL, fibonacci, (void*)&n);	
	pthread_join(t_id, NULL);
	printf("Output : ");
	for (i = 0; i <= n; ++i) {	// fibonacci 값들 출력
		if (i == n)
			printf("%lld\n", arr[i]);
		else
			printf("%lld,", arr[i]);
	}
	endTime = clock(); // 끝나는 시각
	nProcessExcuteTime = ((double)(endTime - startTime)) / CLOCKS_PER_SEC;
	printf("Excute time: %f\n", nProcessExcuteTime);
	///////////////////////////////////	일반 재귀함수로 호출하였을 시
	memset(arr, 0, sizeof(arr));	// arr의 값을 초기화
	printf("\n-----Only recursive\n");
	startTime = clock();	//현재 시각;
	f(n + 1);				// --n을 했었으므로 하나 증가한 값
	printf("Output : ");
	for (i = 0; i <= n; ++i) {	// fibonacci 값들 출력
		if (i == n)
			printf("%lld\n", arr[i]);
		else
			printf("%lld,", arr[i]);
	}
	endTime = clock(); // 끝나는 시각
	nProcessExcuteTime = ((double)(endTime - startTime)) / CLOCKS_PER_SEC;
	printf("Excute time: %f\n", nProcessExcuteTime);

	return 0;
}


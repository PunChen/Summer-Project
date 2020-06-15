/*
*Name : vectorized.c
*Purpose: test the performance of intel compiler
*Author: Albert
*/
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

const int MAX = 2000000;
const int ITERMAX = 20000;

int main(void) {

    int i, iter, arr[MAX];
    timeval start, end;
    long long tPassed = 0;
    memset(arr, 0, sizeof(arr));
    gettimeofday(&start, 0);
    for (iter = 1; iter < ITERMAX; iter++) {
//		for(i = 1; i < MAX; i++){
//			arr[i] = arr[i-1] + 1;
//		}
        for (i = 1; i < MAX; i++) {
            arr[i] += 1;
        }
    }
    gettimeofday(&end, 0);
    end.tv_sec -= start.tv_sec;
    end.tv_usec -= start.tv_usec;
    tPassed = 1000000LL * end.tv_sec + end.tv_usec;
    tPassed /= 1000;
    printf("%lld ms\n", tPassed);
    return 0;
}

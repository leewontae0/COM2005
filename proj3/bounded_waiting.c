/*
 * Copyright(c) 2021-2024 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

#define N 8             /* 스레드 개수 */
#define RUNTIME 100000  /* 출력량을 제한하기 위한 실행시간 (마이크로초) */

/*
 * ANSI 컬러 코드: 출력을 쉽게 구분하기 위해서 사용한다.
 * 순서대로 BLK, RED, GRN, YEL, BLU, MAG, CYN, WHT, RESET을 의미한다.
 */
char *color[N+1] = {"\e[0;30m","\e[0;31m","\e[0;32m","\e[0;33m","\e[0;34m","\e[0;35m","\e[0;36m","\e[0;37m","\e[0m"};

/*
 * waiting[i]는 스레드 i가 임계구역에 들어가기 위해 기다리고 있음을 나타낸다.
 * alive 값이 false가 될 때까지 스레드 내의 루프가 무한히 반복된다.
 */
bool waiting[N];
bool alive = true;

/*
 * 임계구역 내 스레드간 상호 배타 접근을 보장하기 위한 변수
 */
atomic_int lock = 0;

/*
 * N 개의 스레드가 임계구역에 배타적으로 들어가기 위해 스핀락을 사용하여 동기화한다.
 */
void *worker(void *arg)
{
    int i = *(int *)arg;

    /*
     * lock값과 expected값이 같다면, 임계구역에 스레드가 존재하지 않는다는 것을 의미한다.
     */
    int expected = 0; 
    
    while (alive) {

        /*
         * 임계구역에 들어가기 위해 기다리고 있다.
         */
        waiting[i] = true;

        /*
         * 스핀락을 통해 lock을 점유한다.
         * 기다리는 상태이면서, lock을 점유하지 못했다면 임계구역으로 들어갈 수 없다.
         * 기다리지 않는 상태라는 것은, 다른 스레드에 의해 lock을 상속받은 것이다.
         */
        while (waiting[i] && !atomic_compare_exchange_weak(&lock, &expected, 1)) {
            expected = 0;
        }

        /*
         * lock을 얻었으므로, 임계구역에 들어가기 위해 기다리고 있지 않다.
         */
        waiting[i] = false;

        /*
         * 임계구역: 알파벳 문자를 한 줄에 40개씩 10줄 출력한다.
         */
        for (int k = 0; k < 400; ++k) {
            printf("%s%c%s", color[i], 'A'+i, color[N]);
            if ((k+1) % 40 == 0)
                printf("\n");
        }
        
        /*
         * 임계구역이 성공적으로 종료되었다.
         */

        /*
         * 현 스레드 이후의 기다리고 있는 상태의 스레드를 찾는다.
         * 찾았다면 기다리고 있지 않은 상태로 변경한다.
         * 찾지 못했다면, lock을 놓아 준다.
         */
        int j = (i+1) % N;

        while ((j != i) && !waiting[j]) {
            j = (j+1) % N;
        }

        if (i == j) {
            lock = 0;
        } else {
            waiting[j] = false;
        }
    }
    pthread_exit(NULL);
}

int main(void)
{
    pthread_t tid[N];
    int i, id[N];

    /*
     * N 개의 자식 스레드를 생성한다.
     */
    for (i = 0; i < N; ++i) {
        id[i] = i;
        pthread_create(tid+i, NULL, worker, id+i);
    }
    /*
     * 스레드가 출력하는 동안 RUNTIME 마이크로초 쉰다.
     * 이 시간으로 스레드의 출력량을 조절한다.
     */
    usleep(RUNTIME);
    /*
     * 스레드가 자연스럽게 무한 루프를 빠져나올 수 있게 한다.
     */
    alive = false;
    /*
     * 자식 스레드가 종료될 때까지 기다린다.
     */
    for (i = 0; i < N; ++i)
        pthread_join(tid[i], NULL);
    /*
     * 메인함수를 종료한다.
     */
    return 0;
}

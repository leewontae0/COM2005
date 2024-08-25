#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#define L0 8192
#define L1 70
#define L2 70
#define L3 65
#define L4 50
#define L5 70
#define NREAD 20
#define NWRITE 5
#define RUNTIME 200000000L
#define SLEEPTIME 100000

char *img1[L1] = {};
char *img2[L2] = {};
char *img3[L3] = {};
char *img4[L4] = {};
char *img5[L5] = {};

bool alive = true;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 뮤텍스 초기화
pthread_cond_t r_cond = PTHREAD_COND_INITIALIZER;  // 읽기 조건 변수 초기화
pthread_cond_t w_cond = PTHREAD_COND_INITIALIZER;  // 쓰기 조건 변수 초기화

int reader_in, writer_in = 0; // reader_in와 writer_in 변수 초기화
int writer_wait = 0;    // 대기 중인 writer 스레드 수 초기화

void *reader(void *arg)
{
    int id, i;
    id = *(int *)arg;

    while (alive) {
        pthread_mutex_lock(&mutex);             // 락을 얻고 뮤텍스 잠금
        while (writer_in || writer_wait)        // 쓰기 중이거나 대기 중인 writer 스레드가 있으면 대기
            pthread_cond_wait(&r_cond, &mutex); // 읽기 조건 변수 신호 대기, writer가 쓰기를 끝내면 반복문 조건 확인 후 진행

        reader_in++;                    // 읽기 카운트 증가
        pthread_mutex_unlock(&mutex); // 중복하여 출력하기 위해 뮤텍스 잠금 해제

        // 출력
        printf("<");
        for (i = 0; i < L0; ++i)
            printf("%c", 'A' + id);
        printf(">");

        pthread_mutex_lock(&mutex); // 뮤텍스 잠금
        reader_in--;                  // 읽기 카운트 감소

        if (reader_in == 0)                 // 마지막 reader 스레드라면
            pthread_cond_signal(&w_cond); // 쓰기 조건 변수 신호 보냄
        pthread_mutex_unlock(&mutex);     // 뮤텍스 잠금 해제
    }
    pthread_exit(NULL);
}

void *writer(void *arg)
{
    int id, i;
    struct timespec req;
    id = *(int *)arg;
    srand(time(NULL));

    while (alive) {
        pthread_mutex_lock(&mutex); // 뮤텍스 잠금
        writer_wait++;            // 대기 중인 writer 스레드 수 증가

        // 읽기 중이거나 쓰기 중인 스레드가 있으면 대기
        while (reader_in || writer_in)
            pthread_cond_wait(&w_cond, &mutex); // 쓰기 조건 변수 신호 대기

        writer_wait--; // 대기 중인 writer 스레드 수 감소
        writer_in++;       // 쓰기 카운트 증가
        pthread_mutex_unlock(&mutex);

        printf("\n");
        switch (id)
        {
        case 0:
            for (i = 0; i < L1; ++i)
                printf("%s\n", img1[i]);
            break;
        case 1:
            for (i = 0; i < L2; ++i)
                printf("%s\n", img2[i]);
            break;
        case 2:
            for (i = 0; i < L3; ++i)
                printf("%s\n", img3[i]);
            break;
        case 3:
            for (i = 0; i < L4; ++i)
                printf("%s\n", img4[i]);
            break;
        case 4:
            for (i = 0; i < L5; ++i)
                printf("%s\n", img5[i]);
            break;
        default:;
        }

        pthread_mutex_lock(&mutex);          // 뮤텍스 잠금
        writer_in--;                           // 쓰기 카운트 감소
        
        if (writer_wait)                   // 대기 중인 쓰기 스레드가 있으면
            pthread_cond_signal(&w_cond);    // 쓰기 조건 변수 신호
        else                                 // 대기 중인 쓰기 스레드가 없으면
            pthread_cond_broadcast(&r_cond); // 모든 읽기 조건 변수 신호, reader 스레드는 중복하여 출력 가능하므로 모두 한번에 깨움
        pthread_mutex_unlock(&mutex);        // 뮤텍스 잠금 해제

        req.tv_sec = 0;
        req.tv_nsec = rand() % SLEEPTIME;
        nanosleep(&req, NULL);
    }
    pthread_exit(NULL);
}

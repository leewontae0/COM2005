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

/**
 * reader_writer_ticket_mutex	: 전체대기표
 * reader_ticket_mutex			: R대기표
 * writer_ticket_mutex			: W대기표
 * reader_mutex					: R간 상호배타
 * writer_mutex					: W간 상호배타
*/
pthread_mutex_t reader_writer_ticket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reader_ticket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writer_ticket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reader_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writer_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * reader_count	: 임계구역에 진입해있는 R수
*/
int reader_count = 0;



void *reader(void *arg)
{
    int id, i;
    id = *(int *)arg;

    while (alive) {
		/**
		 * 전체대기표를 뽑는다*
		 * R간 상호배타 시작
		 * (다음 RW가 들어올 수 있게) 전체대기표를 폐기한다*
		 * R수를 증가시킨다
		 * 만일 첫 번째 R이면
		 * ....(W를 차단하기 위해) W대기표를 뽑는다
		 * R간 상호배타 종료
		*/
		pthread_mutex_lock(&reader_writer_ticket_mutex);
		pthread_mutex_lock(&reader_mutex);
		pthread_mutex_unlock(&reader_writer_ticket_mutex);
		reader_count++;
		if (reader_count == 1)
			pthread_mutex_lock(&writer_ticket_mutex);
		pthread_mutex_unlock(&reader_mutex);


        // 출력
        printf("<");
        for (i = 0; i < L0; ++i)
            printf("%c", 'A' + id);
        printf(">");

		/**
		 * R간 상호배타 시작
		 * R수를 감소시킨다
		 * 만일 마지막 R이면
		 * ....(W를 허용하기 위해) W대기표를 폐기한다
		 * R간 상호배타 종료
		*/
		pthread_mutex_lock(&reader_mutex);
		reader_count--;
		if (reader_count == 0)
			pthread_mutex_unlock(&writer_ticket_mutex);
		pthread_mutex_unlock(&reader_mutex);
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
		/**
		 * 전체대기표를 뽑는다*
		 * W대기표를 뽑는다
		 * (다음 RW가 들어올 수 있게) 전체대기표를 폐기한다*
		*/
        pthread_mutex_lock(&reader_writer_ticket_mutex);
		pthread_mutex_lock(&writer_ticket_mutex);
		pthread_mutex_unlock(&reader_writer_ticket_mutex);

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
		/**
		 * (다음 W가 들어올 수 있게) W대기표를 폐기한다
		*/
		pthread_mutex_unlock(&writer_ticket_mutex);



        req.tv_sec = 0;
        req.tv_nsec = rand() % SLEEPTIME;
        nanosleep(&req, NULL);
    }
    pthread_exit(NULL);
}

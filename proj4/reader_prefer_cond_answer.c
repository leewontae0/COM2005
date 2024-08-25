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
char *img1[L2] = {};
char *img1[L3] = {};
char *img1[L4] = {};
char *img2[L5] = {};

/*
 * alive 값이 true이면 각 스레드는 무한 루프를 돌며 반복해서 일을 하고,
 * alive 값이 false가 되면 무한 루프를 빠져나와 스레드를 자연스럽게 종료한다.
 */
bool alive = true;

/**
 * writer_in	:	임계구역에 들어가있는 writer의 수
 * reader_in	:	임계구역에 들어가있는 reader의 수
 * reader_wait	:	reader_cond에서 대기하고있는 reader의 수
*/
int writer_in = 0, reader_in = 0, reader_wait = 0;

/**
 * reader_count를 원자적으로 계산할 수 있게 하는 뮤텍스
*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * reader가 대기하게 되는 상태변수.
 * writer_in이 0이 아니면 대기하게 된다.
 * 임계구역에 들어가있는 writer를 기다리는 역할이다.
*/
pthread_cond_t reader_cond = PTHREAD_COND_INITIALIZER;

/**
 * writer가 대기하게 되는 상태변수.
 * reader_in이 0이 아니면 대기하게 된다.
 * 임계구역에 들어가있는 reader를 기다리는 역할이다.
*/
pthread_cond_t writer_cond = PTHREAD_COND_INITIALIZER;
/*
 * Reader 스레드는 같은 문자를 L0번 출력한다. 예를 들면 <AAA...AA> 이런 식이다.
 * 출력할 문자는 인자를 통해 0이면 A, 1이면 B, ..., 등으로 출력하며, 시작과 끝을 <...>로 나타낸다.
 * 단일 reader라면 <AAA...AA>처럼 같은 문자만 출력하겠지만, critical section에서 reader의
 * 중복을 허용하기 때문에 reader가 많아지면 출력이 어지럽게 섞여서 나오는 것이 정상이다.
 */
void *reader(void *arg)
{
    int id, i;

    /*
     * 들어온 인자를 통해 출력할 문자의 종류를 정한다.
     */
    id = *(int *)arg;
    /*
     * 스레드가 살아 있는 동안 같은 문자열 시퀀스 <XXX...XX>를 반복해서 출력한다.
     */
    while (alive)
    {
		/**
		 * reader_wait와 reader_in변수를 수정하고, 조건변수 사용을 위해 뮤텍스 락을 걸어준다.
		 * 대기중인 reader수를 늘려준다.
		 * 임계구역에 들어가있는 writer가 있다면 대기한다.
		 * 조건변수를 빠져나오면 대기중인 reader_wait수를 1 감소시킨다.
		 * 임계구역에 들어가있는 reader_in수를 1 증가시킨다.
		 * 조건변수 사용을 위한 변수 수정이 종료되었으므로 뮤텍스 락을 풀어준다.
		*/
		pthread_mutex_lock(&mutex);
		reader_wait++;
		while (writer_in != 0)
			pthread_cond_wait(&reader_cond, &mutex);
		reader_wait--;
		reader_in++;
		pthread_mutex_unlock(&mutex);
        /*
         * Begin Critical Section
         */
        printf("<");
        for (i = 0; i < L0; ++i)
            printf("%c", 'A' + id);
        printf(">");
        /*
         * End Critical Section
         */
		/**
		 * reader_in 변수를 수정하고, 조건변수 사용을 위해 뮤텍스 락을 걸어준다.
		 * 임계구역이 끝났으므로 임계구역에 들어가있는 reader수를 감소시켜준다.
		 * reader_in이 0이 되면 자신이 마지막 reader이므로 writer하나를 깨워준다.
		 * 조건변수 사용을 위한 변수 수정이 종료되었으므로 뮤텍스 락을 풀어준다.
		*/
		pthread_mutex_lock(&mutex);
		reader_in--;
		if (reader_in == 0)
			pthread_cond_signal(&writer_cond);
		pthread_mutex_unlock(&mutex);

	}
    pthread_exit(NULL);
}

/*
 * Writer 스레드는 어떤 사람의 얼굴 이미지를 출력한다.
 * 이미지는 여러 종류가 있으며 인자를 통해 식별한다.
 * Writer가 critical section에 있으면 다른 writer는 물론이고 어떠한 reader도 들어올 수 없다.
 * 만일 이것을 어기고 다른 writer나 reader가 들어왔다면 얼굴 이미지가 깨져서 쉽게 감지된다.
 */
void *writer(void *arg)
{
    int id, i;
    struct timespec req;

    /*
     * 들어온 인자를 통해 얼굴 이미지의 종류를 정한다.
     * 랜덤 생성기의 시드 값을 현재 시간으로 초기화한다.
     */
    id = *(int *)arg;
    srand(time(NULL));
    /*
     * 스레드가 살아 있는 동안 같은 이미지를 반복해서 출력한다.
     */
    while (alive)
    {
		/**
		 * reader_wait, reader_in, writer_in 값을 사용하고,
		 * 조건변수를 사용하기 위해 뮤텍스 락을 걸어준다.
		 * 대기중인 reader가 있거나,
		 * 임계구역에 있는 reader가 있거나,
		 * 임계구역에 있는 writer가 있으면 대기한다.
		 * 각각 reader_wait != 0, reader_in != 0, writer_in != 0을 의미함.
		 * 
		 * 조건변수에서 나오면 임계구역에 들어가기 전 writer_in값을 1 증가시킨다.
		 * 조건변수 사용을 위한 변수 수정이 종료되었으므로 뮤텍스 락을 풀어준다.
		*/
		pthread_mutex_lock(&mutex);
		while (reader_wait != 0 || reader_in != 0 || writer_in != 0)
			pthread_cond_wait(&writer_cond, &mutex);
		writer_in++;
		pthread_mutex_unlock(&mutex);
        /*
         * Begin Critical Section
         */
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
        /*
         * End Critical Section
         */
		/**
		 * writer_in, reader_wait을 사용하고, 
		 * 조건변수를 사용하기 위해 뮤텍스 락을 걸어준다.
		 * 임계구역에서 나왔으므로 writer_in값을 1 감소시켜준다.
		 * 자신이 마지막 writer이고, 대기중인 reader가 있으면 모든 reader의 대기를 풀어준다.
		 * 자신이 마지막 writer이고, 대기중인 reader가 없으면 writer하나의 대기를 풀어준다.
		 * 조건변수 사용을 위한 변수 수정이 종료되었으므로 뮤텍스 락을 풀어준다.
		*/
		pthread_mutex_lock(&mutex);
		writer_in--;
		if (writer_in == 0 && reader_wait != 0)
			pthread_cond_broadcast(&reader_cond);
		if (writer_in == 0 && reader_wait == 0)
			pthread_cond_signal(&writer_cond);
		pthread_mutex_unlock(&mutex);
        /*
         * 이미지 출력 후 SLEEPTIME 나노초 안에서 랜덤하게 쉰다.
         */
        req.tv_sec = 0;
        req.tv_nsec = rand() % SLEEPTIME;
        nanosleep(&req, NULL);
    }
    pthread_exit(NULL);
}

/*
 * 메인 함수는 NREAD 개의 reader 스레드를 생성하고, NWRITE 개의 writer 스레드를 생성한다.
 * 생성된 스레드가 일을 할 동안 0.2초 동안 기다렸다가 alive의 값을 0으로 바꿔서 모든 스레드가
 * 무한 루프를 빠져나올 수 있게 만든 후, 스레드가 자연스럽게 종료할 때까지 기다리고 메인을 종료한다.
 */
int main(void)
{
    int i;
    int rarg[NREAD], warg[NWRITE];
    pthread_t rthid[NREAD];
    pthread_t wthid[NWRITE];
    struct timespec req;

    /*
     * Create NREAD reader threads
     */
    for (i = 0; i < NREAD; ++i)
    {
        rarg[i] = i;
        if (pthread_create(rthid + i, NULL, reader, rarg + i) != 0)
        {
            fprintf(stderr, "pthread_create error\n");
            return -1;
        }
    }
    /*
     * Create NWRITE writer threads
     */
    for (i = 0; i < NWRITE; ++i)
    {
        warg[i] = i;
        if (pthread_create(wthid + i, NULL, writer, warg + i) != 0)
        {
            fprintf(stderr, "pthread_create error\n");
            return -1;
        }
    }
    /*
     * Wait for RUNTIME nanoseconds while the threads are working
     */
    req.tv_sec = 0;
    req.tv_nsec = RUNTIME;
    nanosleep(&req, NULL);
    /*
     * Now terminate all threads and leave
     */
    alive = false;
    for (i = 0; i < NREAD; ++i)
        pthread_join(rthid[i], NULL);
    for (i = 0; i < NWRITE; ++i)
        pthread_join(wthid[i], NULL);

    return 0;
}

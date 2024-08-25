/*
 * Copyright(c) 2021-2024 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */
#include <stdlib.h>
#include "pthread_pool.h"

/*
 * 풀에 있는 일꾼(일벌) 스레드가 수행할 함수이다.
 * FIFO 대기열에서 기다리고 있는 작업을 하나씩 꺼내서 실행한다.
 * 대기열에 작업이 없으면 새 작업이 들어올 때까지 기다린다.
 * 이 과정을 스레드풀이 종료될 때까지 반복한다.
 */
static void *worker(void *param)
{
    task_t task;
    pthread_pool_t *pool = (pthread_pool_t *)param;

    while (true) {

        pthread_mutex_lock(&pool->mutex_s);

        if (pool->state == 2) {
            pthread_mutex_unlock(&pool->mutex_s);
            break;
        }

        pthread_mutex_unlock(&pool->mutex_s);

        pthread_mutex_lock(&pool->mutex);

        while (pool->q_len == 0) {

            pthread_mutex_lock(&pool->mutex_s);
            if (pool->state) {
                pthread_mutex_unlock(&pool->mutex_s);
                break;
            }
            pthread_mutex_unlock(&pool->mutex_s);

            pthread_cond_wait(&pool->full, &pool->mutex);
        }


        pthread_mutex_lock(&pool->mutex_s);
        if (pool->q_len == 0 && pool->state) {
            pthread_mutex_unlock(&pool->mutex);
            pthread_mutex_unlock(&pool->mutex_s);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&pool->mutex_s);

        task = pool->q[pool->q_front];
        pool->q_front = (pool->q_front + 1) % pool->q_size;
        --(pool->q_len);
        
        pthread_cond_signal(&pool->empty);
        pthread_mutex_unlock(&pool->mutex);

        task.function(task.param);
    }

    pthread_exit(NULL);
}

/*
 * 스레드풀을 생성한다. bee_size는 일꾼(일벌) 스레드의 개수이고, queue_size는 대기열의 용량이다.
 * bee_size는 POOL_MAXBSIZE를, queue_size는 POOL_MAXQSIZE를 넘을 수 없다.
 * 일꾼 스레드와 대기열에 필요한 공간을 할당하고 변수를 초기화한다.
 * 일꾼 스레드의 동기화를 위해 사용할 상호배타 락과 조건변수도 초기화한다.
 * 마지막 단계에서는 일꾼 스레드를 생성하여 각 스레드가 worker() 함수를 실행하게 한다.
 * 대기열로 사용할 원형 버퍼의 용량이 일꾼 스레드의 수보다 작으면 효율을 극대화할 수 없다.
 * 이런 경우 사용자가 요청한 queue_size를 bee_size로 상향 조정한다.
 * 성공하면 POOL_SUCCESS를, 실패하면 POOL_FAIL을 리턴한다.
 */
int pthread_pool_init(pthread_pool_t *pool, size_t bee_size, size_t queue_size)
{
    if (bee_size > POOL_MAXBSIZE || queue_size > POOL_MAXQSIZE) {
        return POOL_FAIL;
    }
    if (queue_size < bee_size) {
        queue_size = bee_size;
    }

    pool->bee = (pthread_t *) malloc(sizeof(pthread_t) * bee_size);
    pool->q = (task_t *) malloc(sizeof(task_t) * queue_size);

    pool->state = 0;
    pool->q_size = queue_size;
    pool->q_front = 0;
    pool->q_len = 0;
    pool->bee_size = bee_size;
    
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_mutex_init(&pool->mutex_s, NULL);
    pthread_cond_init(&pool->full, NULL);
    pthread_cond_init(&pool->empty, NULL);

    for (int i = 0; i < bee_size; ++i) {
        pthread_create(&pool->bee[i], NULL, worker, pool);
    }

    return POOL_SUCCESS;    
}

/*
 * 스레드풀에서 실행시킬 함수와 인자의 주소를 넘겨주며 작업을 요청한다.
 * 스레드풀의 대기열이 꽉 찬 상황에서 flag이 POOL_NOWAIT이면 즉시 POOL_FULL을 리턴한다.
 * POOL_WAIT이면 대기열에 빈 자리가 나올 때까지 기다렸다가 넣고 나온다.
 * 작업 요청이 성공하면 POOL_SUCCESS를 그렇지 않으면 POOL_FAIL을 리턴한다.
 */
int pthread_pool_submit(pthread_pool_t *pool, void (*f)(void *p), void *p, int flag)
{
    pthread_mutex_lock(&pool->mutex);

    if (pool->q_len == pool->q_size) { // 만약, 대기열이 가득 찼다면

        if (flag == POOL_NOWAIT) {
            pthread_mutex_unlock(&pool->mutex);
            return POOL_FULL;
        }
        else { // flag == POOL_WAIT
            while (pool->q_len == pool->q_size) {

                pthread_mutex_lock(&pool->mutex_s);
                if (pool->state) {
                    pthread_mutex_unlock(&pool->mutex_s);
                    break;
                }
                pthread_mutex_unlock(&pool->mutex_s);
                
                pthread_cond_wait(&pool->empty, &pool->mutex);
            }
        }
    }

    pthread_mutex_lock(&pool->mutex_s);
    if (pool->state) {
        pthread_mutex_unlock(&pool->mutex);
        pthread_mutex_unlock(&pool->mutex_s);
        return POOL_FAIL;
    }
    pthread_mutex_unlock(&pool->mutex_s);

    task_t task;
    task.function = f;
    task.param = p;

    pool->q[(pool->q_front + pool->q_len) % pool->q_size] = task;
    ++(pool->q_len);

    pthread_cond_signal(&pool->full);
    pthread_mutex_unlock(&pool->mutex);

    return POOL_SUCCESS;

}

/*
 * 스레드풀을 종료한다. 일꾼 스레드가 현재 작업 중이면 그 작업을 마치게 한다.
 * how의 값이 POOL_COMPLETE이면 대기열에 남아 있는 모든 작업을 마치고 종료한다.
 * POOL_DISCARD이면 대기열에 새 작업이 남아 있어도 더 이상 수행하지 않고 종료한다.
 * 메인(부모) 스레드는 종료된 일꾼 스레드와 조인한 후에 스레드풀에 할당된 자원을 반납한다.
 * 스레드를 종료시키기 위해 철회를 생각할 수 있으나 바람직하지 않다.
 * 락을 소유한 스레드를 중간에 철회하면 교착상태가 발생하기 쉽기 때문이다.
 * 종료가 완료되면 POOL_SUCCESS를 리턴한다.
 */
int pthread_pool_shutdown(pthread_pool_t *pool, int how)
{

    pthread_mutex_lock(&pool->mutex_s);

    if (how == POOL_COMPLETE) {
        pool->state = 1;  
    } else {
        pool->state = 2;
    }

    pthread_mutex_unlock(&pool->mutex_s);

    pthread_mutex_lock(&pool->mutex);

    pthread_cond_broadcast(&pool->full);
    pthread_cond_broadcast(&pool->empty);

    pthread_mutex_unlock(&pool->mutex);

    for (int i = 0; i < pool->bee_size; ++i) {
        pthread_join(pool->bee[i], NULL);
    }   

    free(pool->bee);
    free(pool->q);

    return POOL_SUCCESS;
}

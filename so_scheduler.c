#include "so_scheduler.h"
#include <stdlib.h>
#include <semaphore.h>

#define SO_MAX_UNITS 32
#define SO_MAX_THREADS 50

typedef struct thread {
    tid_t id;
    unsigned int priority;
    unsigned int time;
    unsigned int io;
    unsigned int state;  //1 -> ready; 2 -> running; 3 -> waiting; 4 -> terminated
    so_handler* func;
    sem_t* semafor;
} thread;

typedef struct scheduler {
    unsigned int time_quantum;
    unsigned int io;
    thread** ready;
    thread* running;
    thread** waiting;
    thread** terminated;
    unsigned int no_threads;
    unsigned int no_threads_ready;
} scheduler;

scheduler* sched = NULL;

int so_init(unsigned int time_quantum, unsigned int io)
{
    if(sched != NULL) return -1;
    if(time_quantum <= SO_MAX_UNITS && time_quantum != 0 && io <= SO_MAX_NUM_EVENTS) {
        sched = calloc(1, sizeof(scheduler));
        sched->time_quantum = time_quantum;
        sched->io = io;
        sched->ready = calloc(SO_MAX_THREADS, sizeof(thread*));
        sched->waiting = calloc(SO_MAX_THREADS, sizeof(thread*));
        sched->terminated = calloc(SO_MAX_THREADS, sizeof(thread*));
        sched->no_threads = 0;
        sched->no_threads_ready = 0;
    
        return 0;
    }

    return -1;
}

void* start_thread(void* thr)
{
    thread* t = (thread*)thr;
    
    sem_wait(t->semafor);
    t->func(t->priority);
    
    return NULL;
}

void insert_in_ready(thread* t)
{
    t->time = sched->time_quantum;
    int j = sched->no_threads_ready;
    *(sched->ready + j) = t;
    int i = 0;

    if(*(sched->ready) != NULL) {
        for(i = 0; i < sched->no_threads_ready; i++) {
            if(t->priority > (*(sched->ready + i))->priority) break;
        }

        for(int j = sched->no_threads_ready - 1; j >= i; j--) {
            *(sched->ready + j) = *(sched->ready + j - 1);
        }
    }

    *(sched->ready + i) = t;
    sched->no_threads_ready++;
}

void delete_from_ready()
{
    int i = 0;
    while(i < sched->no_threads_ready - 1) {
        *(sched->ready + i) = *(sched->ready + i + 1);
        i++;
    }

    //free(*(sched->ready + sched->no_threads_ready));
    sched->no_threads_ready--;
}

void add(thread* t)
{
    // daca threadul pe care il inserez are prioritate mai mare decat cel de pe running
    if(sched->running->priority < t->priority) {
        thread* aux = sched->running;
        sched->running = t;
        insert_in_ready(aux);
        sem_post(t->semafor);
        sem_wait(aux->semafor);
    } else {
        sched->running->time--;
        insert_in_ready(t);
    }
}

tid_t so_fork(so_handler *func, unsigned int priority)
{
    if(func == NULL || priority > SO_MAX_PRIO || sched == NULL) return INVALID_TID;

    thread* t = calloc(1, sizeof(thread));
    sched->no_threads++;
    t->id = sched->no_threads;
    t->priority = priority;
    t->io = 0;
    t->time = sched->time_quantum;
    t->func = func;
    t->semafor = calloc(1, sizeof(sem_t));
    sem_init(t->semafor, 0, 0);
    pthread_create(&t->id, NULL, &start_thread, (void*)t);

    if(sched->no_threads == 1) {
        t->state = 2;
        sched->running = t;
        sem_post(t->semafor);
    } else {
        add(t);
        if(sched->running->time == 0) {
            if(*(sched->ready) != NULL && sched->running->priority == (*(sched->ready))->priority) {
                thread* aux = sched->running;
                sched->running = *(sched->ready);
                delete_from_ready();
                insert_in_ready(aux);
                sem_post(sched->running->semafor);
                sem_wait(aux->semafor);
                return 0;
            } else sched->running->time = sched->time_quantum;
        }
    }

    return t->id;
}

int so_wait(unsigned int io)
{
    if(io >= sched->io) return -1;

    return 0;
}

int so_signal(unsigned int io)
{
    if(io < sched->io) return io;

    return -1;
}

void so_exec(void)
{
    if(sched->running != NULL) {
        sched->running->time--;
        if(sched->running->time <= 0) {
            if(*(sched->ready) != NULL && sched->running->priority == (*(sched->ready))->priority) {
                //if(sched->running->priority == (*(sched->ready))->priority) {
                    thread* aux = sched->running;
                    sched->running = *(sched->ready);
                    delete_from_ready();
                    insert_in_ready(aux);
                    sem_post(sched->running->semafor);
                    sem_wait(aux->semafor);
                //}
            } else sched->running->time = sched->time_quantum;
        }
    }
}

void so_end(void)
{
    if(sched) {
        if(sched->running) {
            pthread_join(sched->running->id, NULL);
            sem_destroy(sched->running->semafor);
            free(sched->running->semafor);
            free(sched->running);
            for(int i = 0; i < sched->no_threads_ready; i++) {
                sched->running = *(sched->ready + i);
                sem_post(sched->running->semafor);
                pthread_join(sched->running->id, NULL);
                free((*(sched->ready + i))->semafor);
                free(*(sched->ready + i));
                *(sched->ready + i) = NULL;
            }
            sched->running = NULL;
        }
        if(sched->ready) {
            free(sched->ready);
            sched->ready = NULL;
        }
        if(sched->waiting) {
            free(sched->waiting);
            sched->waiting = NULL;
        }
        if(sched->terminated) {
            free(sched->terminated);
            sched->terminated = NULL;
        }

        free(sched);
        sched = NULL;
    }
}
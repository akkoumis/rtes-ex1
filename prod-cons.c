/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.	
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#define QUEUESIZE 10
#define LOOP 2000
#define pNum 1
#define qNum 4

void *producer(void *tid);

void *consumer(void *tid);

typedef struct {
    void *(*work)(void *);

    void *arg;
    struct timeval *tv;
} workFunction;

// Structure representing the queue
typedef struct {
    workFunction **buf; // Queue buffer
    long head, tail; // Boundries
    int full, empty; // Flags for empty, full
    pthread_mutex_t *mut; // Mutex for modifying the queue
    pthread_cond_t *notFull, *notEmpty; // Condition variables for
} queue;

queue *fifo; // The queue
int areProducersActive; // Flag whether there is at least one producer thread active
FILE *fp;

queue *queueInit(void);

void queueDelete(queue *q);

void queueAdd(queue *q, workFunction *in);

void queueDel(queue *q, workFunction **out);

int main() {
    pthread_t pro[pNum], con[qNum]; // Producer and Consumer thread TODO Convert to array

    fifo = queueInit(); // Initialize the queue, using the function
    if (fifo == NULL) {
        fprintf(stderr, "main: Queue Init failed.\n");
        exit(1);
    }

    // File Creation
    __time_t timestamp;
    time(&timestamp);
    char buffer[25], name[50];
    struct tm *info = localtime(&timestamp);
    strftime(buffer, 25, "%Y_%m_%d_%H_%M_%S", info);
    sprintf(name, "../stats/%s_p_%d_q_%d.txt", buffer, pNum, qNum);
    //printf("timestamp: %s\n", name);
    fp = fopen(name, "w+");
    if (fp == NULL) {
        fprintf(stderr, "main: File Open failed.\n");
        exit(2);
    }

    // Thread Creation
    for (int tid = 0; tid < pNum; ++tid) {
        pthread_create(&pro[tid], NULL, producer, tid); // Create the Producer thread
    }
    areProducersActive = 1;
    for (int tid = 0; tid < qNum; ++tid) {
        pthread_create(&con[tid], NULL, consumer, tid); // Create the Consumer thread
    }

    // Thread Join
    for (int tid = 0; tid < pNum; ++tid) {
        pthread_join(pro[tid], NULL); // Join  the Producer thread to main thread and wait for its completion
    }
    areProducersActive = 0;
    pthread_mutex_lock(fifo->mut);
    pthread_cond_broadcast(fifo->notEmpty); // In case any of the consumers is condition waiting
    printf("BROADCAST for possible conditional waiting consumers!!!\n");
    pthread_mutex_unlock(fifo->mut);
    for (int tid = 0; tid < qNum; ++tid) {
        pthread_join(con[tid], NULL); // Join  the Consumer thread to main thread and wait for its completion
    }
    queueDelete(fifo);

    fclose(fp);

    return 0;
}

void *producer(void *tid) {
    //queue *fifo;
    int i;

    //fifo = (queue *) q;

    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock(fifo->mut); // Attempt to lock queue mutex.
        while (fifo->full) { // When lock is acquired check if queue is full
            printf("producer %d: queue FULL.\n", (int) tid);
            pthread_cond_wait(fifo->notFull, fifo->mut); // Conditional wait until queue is full NO MORE
        }
        workFunction *wF = (workFunction *) malloc(sizeof(workFunction)); // workFunction to add malloc
        wF->tv = (struct timeval *) malloc(sizeof(struct timeval));
        gettimeofday((wF->tv), NULL);
        wF->arg = i;
        queueAdd(fifo, wF);
        //printf("++\n");
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
        //pthread_cond_broadcast(fifo->notEmpty);
        //usleep(100000);
    }

    printf("producer %d: RETURNED.\n", (int) tid);
    return (NULL);
}

void *consumer(void *tid) {
    //queue *fifo;
    int i;
    workFunction *d;

    //fifo = (queue *) q;
    while (1) {
        //for (i = 0; i < LOOP; i++) {
        struct timeval now, res;
        pthread_mutex_lock(fifo->mut);
        while (fifo->empty) {
            if (areProducersActive == 0) {
                pthread_mutex_unlock(fifo->mut);
                printf("consumer %d: queue RETURNED.\n", (int) tid);
                return (NULL);
            }
            printf("consumer %d: queue EMPTY.\n", (int) tid);
            pthread_cond_wait(fifo->notEmpty, fifo->mut);

        }
        gettimeofday(&now, NULL);
        queueDel(fifo, &d);
        timersub(&now, (d->tv), &res);
        fprintf(fp, "%ld\n", res.tv_sec * 1000000 + res.tv_usec);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);
//pthread_cond_broadcast(fifo->notFull);
        printf("consumer %d: recieved %d, after %ld.\n", (int) tid, d->arg, res.tv_sec * 1000000 + res.tv_usec);
        free(d->tv);
        free(d); // workFunction to delete free
//usleep(200000);
    }

    //return (NULL);
}

/*
  typedef struct {
  int buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  } queue;
*/

queue *queueInit(void) {
    queue *q;

    q = (queue *) malloc(sizeof(queue));
    if (q == NULL) return (NULL);

    q->buf = (workFunction **) malloc(QUEUESIZE * sizeof(workFunction *)); // Buffer malloc
    if (q->buf == NULL) // If buffer malloc failed then return NULL
        return (NULL);

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->mut, NULL);
    q->notFull = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notEmpty, NULL);

    return (q);
}

void queueDelete(queue *q) {
    pthread_mutex_destroy(q->mut);
    free(q->mut);
    pthread_cond_destroy(q->notFull);
    free(q->notFull);
    pthread_cond_destroy(q->notEmpty);
    free(q->notEmpty);

    free(q->buf); // Buffer free
    free(q);
}

void queueAdd(queue *q, workFunction *in) {
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDel(queue *q, workFunction **out) {
    *out = q->buf[q->head];

    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}
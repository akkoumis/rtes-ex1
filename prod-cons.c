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

#define QUEUESIZE 10
#define LOOP 20
#define pNum 4
#define qNum 4

void *producer(void *args);

void *consumer(void *args);

typedef struct {
    void *(*work)(void *);

    void *arg;
} workFunction;

// Structure representing the queue
typedef struct {
    workFunction buf[QUEUESIZE]; // Queue buffer
    long head, tail; // Boundries
    int full, empty; // Flags for empty, full
    pthread_mutex_t *mut; // Mutex for modifying the queue
    pthread_cond_t *notFull, *notEmpty; // Condition variables for
} queue;

queue *queueInit(void);

void queueDelete(queue *q);

void queueAdd(queue *q, workFunction in);

void queueDel(queue *q, workFunction *out);

int main() {
    queue *fifo; // The queue
    pthread_t pro, con; // Producer and Consumer thread TODO Convert to array

    fifo = queueInit(); // Initialize the queue, using the function
    if (fifo == NULL) {
        fprintf(stderr, "main: Queue Init failed.\n");
        exit(1);
    }
    pthread_create(&pro, NULL, producer, fifo); // Create the Producer thread
    pthread_create(&con, NULL, consumer, fifo); // Create the Consumer thread
    pthread_join(pro, NULL); // Join  the Producer thread to main thread and wait for its completion
    pthread_join(con, NULL); // Join  the Consumer thread to main thread and wait for its completion
    queueDelete(fifo);

    return 0;
}

void *producer(void *q) {
    queue *fifo;
    int i;

    fifo = (queue *) q;

    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock(fifo->mut); // Attempt to lock queue mutex.
        while (fifo->full) { // When lock is acquired check if queue is full
            printf("producer: queue FULL.\n");
            pthread_cond_wait(fifo->notFull, fifo->mut); // Conditional wait until queue is full NO MORE
        }
        workFunction wF;
        wF.arg=i;
        queueAdd(fifo, wF);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
        //pthread_cond_broadcast(fifo->notEmpty);
        usleep(100000);
    }
    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock(fifo->mut);
        while (fifo->full) {
            printf("producer: queue FULL.\n");
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }
        workFunction wF;
        wF.arg=i;
        queueAdd(fifo, wF);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
        //pthread_cond_broadcast(fifo->notEmpty);
        usleep(200000);
    }
    return (NULL);
}

void *consumer(void *q) {
    queue *fifo;
    int i;
    workFunction d;

    fifo = (queue *) q;

    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock(fifo->mut);
        while (fifo->empty) {
            printf("consumer: queue EMPTY.\n");
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }
        queueDel(fifo, &d);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);
        //pthread_cond_broadcast(fifo->notFull);
        printf("consumer: recieved %d.\n", d.arg);
        usleep(200000);
    }
    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock(fifo->mut);
        while (fifo->empty) {
            printf("consumer: queue EMPTY.\n");
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }
        queueDel(fifo, &d);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);
        //pthread_cond_broadcast(fifo->notFull);
        printf("consumer: recieved %d.\n", d.arg);
        usleep(50000);
    }
    return (NULL);
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
    free(q);
}

void queueAdd(queue *q, workFunction in) {
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDel(queue *q, workFunction *out) {
    *out = q->buf[q->head];

    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}
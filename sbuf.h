/* sbuf.h */
#ifndef __SBUF_H__
#define __SBUF_H__

#include "csapp.h"

/* Bounded buffer structure */
typedef struct {
    int *buf;          /* Buffer array */
    int n;             /* Maximum number of slots */
    int front;         /* Index of first item: buf[(front+1)%n] */
    int rear;          /* Index of last item: buf[rear%n] */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;

/* Function prototypes */
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

#endif /* __SBUF_H__ */
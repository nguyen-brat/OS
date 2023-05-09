#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "flag.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        if(q->size == MAX_QUEUE_SIZE) return;
        q->proc[q->size++] = proc; 
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (FLAG) printf("Flag A\n");
        if(empty(q)) return NULL;
        if (FLAG) printf("Flag B\n");
        struct pcb_t* result = q->proc[0];
        if (FLAG) printf("Flag C\n");
        for(int i=q->size-1; i>0; i--){
                if (FLAG) printf("Flag D\n");
                q->proc[i-1] = q->proc[i];
        }
        q->size--;
	return result;
}
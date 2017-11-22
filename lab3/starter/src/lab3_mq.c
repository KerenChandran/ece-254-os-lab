#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <mqueue.h>

int N,B,P,C;
int EMPTY_QUEUE = -1;
struct timeval time_val;
mqd_t may_produce;
mqd_t may_consume;

char *may_produce_qname = "/produce";
char *may_consume_qname = "/consume";
mode_t mode = S_IRUSR | S_IWUSR;

void init(int B) {
    struct mq_attr may_produce_attr;
    struct mq_attr may_consume_attr;
    int initial_count = 0;

    may_produce_attr.mq_maxmsg  = B;
    may_produce_attr.mq_msgsize = sizeof(int);
    may_produce_attr.mq_flags   = 0;		/* a blocking queue  */

    may_consume_attr.mq_maxmsg  = 1;
    may_consume_attr.mq_msgsize = sizeof(int);
    may_consume_attr.mq_flags   = 0;		/* a blocking queue  */


    may_produce = mq_open(may_produce_qname, O_RDWR | O_CREAT, mode, &may_produce_attr);
    if (may_produce == -1 ) {
        perror("mq_open() failed");
        exit(1);
    }

    may_consume = mq_open(may_consume_qname, O_RDWR | O_CREAT, mode, &may_consume_attr);
    if (may_consume == -1 ) {
        perror("mq_open() failed");
        exit(1);
    }

    if (mq_send(may_consume, (char *) &initial_count, sizeof(int), 0) == -1) {
        perror("mq_send() failed");
    }
}

void producer(int p) {
    int next_number = p;
    
    while(next_number <= N) {
        if (mq_send(may_produce, (char *) &next_number, sizeof(int), 0) == -1) {
            perror("mq_send() failed");
        }

        next_number = next_number + P; // next number to produce        
    }
}

void consumer(int id) {
    int num;
    double root;
    bool consumer_continue = true;
    int count;

    while(consumer_continue) {
        mq_receive(may_produce, (char *) &num, sizeof(int), 0);
        mq_receive(may_consume, (char *) &count, sizeof(int), 0);
        if (count <= N) {
            // Calculate square root
            root = sqrt((float)num);
            // Only print perfect roots 
            if(root == (int)root) {
                printf("%d %d %f \n", id, num, root);
            }
        }
        if (count >= N) {
            consumer_continue = false;
            if (mq_send(may_produce, (char *) &EMPTY_QUEUE, sizeof(int), 0) == -1) {
                perror("mq_send() failed");
            }
        }
        count++;
        if (mq_send(may_consume, (char *) &count, sizeof(int), 0) == -1) {
            perror("mq_send() failed");
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Please supply 4 arguments, in the order: <NumIntegers> <BufferSize> <NumProducers> <NumConsumers> \n");
        return 0;
    }

    N = atoi(argv[1]); // number of numbers that producers will produce
    B = atoi(argv[2]); // buffer size
    P = atoi(argv[3]); // number of producers
    C = atoi(argv[4]); // number of consumers

    int status;
    pid_t pids[P];
    pid_t cids[C];
    
    init(B);

    // start counting program duration
    gettimeofday(&time_val, NULL);
    double start_time = ((double) time_val.tv_sec) + time_val.tv_usec/1000000.0;


    // Create processes
    // - create producer
    for (int p = 0; p < P; p++) {
	pids[p] = fork();
        if (pids[p] == 0) {
	    producer(p);
            exit(0);
	}
    }
    // - create consumer
    for (int c = 0; c < C; c++) {
        cids[c] = fork();
        if (cids[c] == 0) {
	    consumer(c);
            exit(0);
	}
    }

    // Collect results
    for (int p = 0; p < P + C; p++) { 
        wait(&status);
        if (!WIFEXITED(status)) {
            return -1;
        }
    }

    if (mq_close(may_consume) == -1 || mq_close(may_produce) == -1) {
        perror("mq_close() failed");
        exit(2);
    }

    if (mq_unlink(may_produce_qname) != 0 || mq_unlink(may_consume_qname) != 0) {
        perror("mq_unlink() failed");
        exit(3);
    }


    gettimeofday(&time_val, NULL);    
    double end_time = ((double) time_val.tv_sec) + time_val.tv_usec/1000000.0;
    printf("System execution time: %f\n", end_time - start_time);
}

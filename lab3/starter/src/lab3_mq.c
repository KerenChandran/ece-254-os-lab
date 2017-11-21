#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <mqueue.h>

int N,B,P,C, p_count, c_count;

int* boundedBuffer;

struct timeval time_val;

mqd_t may_produce;
mqd_t may_consume;
struct mq_attr attr;
mode_t mode = S_IRUSR | S_IWUSR;

void init(int B) {
    char *qname = "/list";
    attr.mq_maxmsg  = B;
    attr.mq_msgsize = sizeof(int);
    attr.mq_flags   = 0;		/* a blocking queue  */

    may_produce  = mq_open(qname, O_RDWR | O_CREAT, mode, &attr);
    if (may_produce == -1 ) {
        perror("mq_open() failed");
        exit(1);
    }

    may_consume  = mq_open(qname, O_RDONLY, mode, &attr);
    if (may_consume == -1 ) {
        perror("mq_open() failed");
        exit(1);
    }

    p_count = 0;
    c_count = 0;
}

void producer(int p) {
    int next_number = p;
    
    while(1) {
        if (next_number > N) {
            break;
        }
        p_count++;

        if (mq_send(may_produce, (char *) &next_number, sizeof(int), 0) == -1) {
            perror("mq_send() failed");
        }

        next_number = next_number + P; // next number to produce        
    }
}

void consumer(int id) {
    int num;
    double root;

    while(1) {
        if (c_count > N - 1) {
            break;
        }
        c_count++;

        // Remove from buffer
        if (mq_receive(may_consume, (char *) &num, \
		    sizeof(int), 0) == -1) {
            perror("mq_timedreceive() failed");
	    printf("Type Ctrl-C and wait for 5 seconds to terminate.\n");

        } else {
            // Calculate square root
            root = sqrt((float)num);
            // Only print perfect roots 
            if(root == (int)root) {
                printf("Consumer id: %d, num: %d, root: %f \n", id, num, root);
            }
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
    // - collect producer results
    for (int p = 0; p < P; p++) { 
        wait(&status);
        if (!WIFEXITED(status)) {
            return -1;
        }
    }

    // - collect consumer results
    for (int c = 0; c < C; c++) {
        wait(&status);
        if (!WIFEXITED(status)) {
            return -1;
        }
    }

    gettimeofday(&time_val, NULL);    
    double end_time = ((double) time_val.tv_sec) + time_val.tv_usec/1000000.0;
    printf("System execution time: %f\n", end_time - start_time);
}

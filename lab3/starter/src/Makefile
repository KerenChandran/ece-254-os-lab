# ECE254 Lab Linux Port
# Makefile: build mqueue sender and receiver example code

CC=gcc
CFLAGS=-Wall -g -lrt
LD=gcc
LDFLAGS=-g -lrt -lm


OBJS1=lab3_mq.o

all: produce

produce: $(OBJS1) 
	$(LD) -o $@ $(OBJS1) $(LDFLAGS) 

%.o: %.c
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	$(CC) -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o *.out 

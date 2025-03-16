# CC=gcc -g -Wall
CC=gcc -Wall
TARGET=hw2
OBJS=testcase.o buf.o disk.o utils.o
# OBJS=main.o buf.o disk.o utils.o

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

.c.o:
	$(CC) -c -o $@ $^

# test:
# 	make clean; make;

# main.o : main.c
# 	$(CC) -c -o $@ $^

# hw1.o : hw1.c
# 	$(CC) -c -o $@ $^

clean: 
	rm *.o $(TARGET)
# CC=gcc -g -Wall
CC=gcc -Wall
TARGET=hw3
# OBJS=main.o buf.o disk.o buf_utils.o fs.o fs_internal.o fs_utils.o
OBJS=testcase.o buf.o disk.o buf_utils.o fs.o fs_internal.o fs_utils.o

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
	rm *.o $(TARGET) MY_DISK
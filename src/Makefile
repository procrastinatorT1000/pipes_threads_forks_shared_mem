CC=g++
#-lrt For shared mem, -pthread for threads
FLAGS=-lrt -pthread

all: task

task: main.o processes.o
	$(CC) main.o processes.o $(FLAGS) -o task

main.o: main.cpp
	$(CC) -c main.cpp

processes.o: processes.cpp
	$(CC) -c processes.cpp

clean:
	rm -rf *.o task


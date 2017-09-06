CC=g++
#-lrt For shared mem, -pthread for threads
FLAGS=-lrt -pthread

all: pipe

pipe: pipe.o
	$(CC) pipe.o $(FLAGS) -o pipe

pipe.o: pipe.cpp
	$(CC) -c pipe.cpp

clean:
	rm -rf *.o pipe


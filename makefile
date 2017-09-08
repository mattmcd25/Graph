all: clean graph

graph: graph.o
	g++ graph.o -o graph -pthread
	
graph.o: graph.cpp
	g++ -c graph.cpp -pthread

clean: 
	rm *.o graph -f

#include <stdlib.h>
#include <vector>
#include <semaphore.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <stack>
#include <sys/types.h>
#include <sys/time.h>
#include <regex.h>
using namespace std;

// the global value to be calculated by the processes
int globalValue = 0;
sem_t mutex;

vector<pthread_t> pthreads;

typedef struct edge_t {
	char dependency;
	char result;
	sem_t sem;
} edge;

// list of all of the edges in the program
vector<edge> edges;

typedef struct node_t {
	char name;
	vector<string> expr;
	int sleep_sec;
	bool needsEval;
} node;

// list of all nodes in the program
vector<node> nodes;

bool isInteger(const char* str) {
	return (atoi(str) != 0 || str[0] == '0');
}

void *compute(void* arg) {
	node *current = (node*)arg;
	// wait for dependencies
	for(int i = 0; i < edges.size(); i++) {
		if(edges[i].result == current->name) {
			sem_wait(&edges[i].sem);
		}
	}

	// compute value
	int comp;
	if(current->needsEval) {
		stack<int> rpnstack;
		string val;
		int v1, v2;
		for(int i = 0; i < current->expr.size(); i++) {
			val = current->expr[i];
			if(isInteger(val.c_str())) rpnstack.push(atoi(val.c_str()));
			else if(val[0] == 'V') {
				sem_wait(&mutex);
				rpnstack.push(globalValue);
				sem_post(&mutex);
			}
			else if(val[0] == 'I') {
				rpnstack.push(current->name-'A');
			}
			else {
				v2 = rpnstack.top();
				rpnstack.pop();
				v1 = rpnstack.top();
				rpnstack.pop();
				switch(val[0]) {
				case '+': rpnstack.push(v1+v2);
						  break;
				case '-': rpnstack.push(v1-v2);
						  break;
				case '*': rpnstack.push(v1*v2);
						  break;
				case '/': rpnstack.push(v1/v2);
						  break;
				case '%': rpnstack.push(v1%v2);
				}
			}
		}
		comp = rpnstack.top();
		rpnstack.pop();
	}
	else comp = atoi(current->expr.front().c_str());
	//int comp = current->value;

	// wait processing time
	sleep(current->sleep_sec);

	// put value into global
	sem_wait(&mutex);
	globalValue += comp;
	sem_post(&mutex);

	// signal any children
	for(int i = 0; i < edges.size(); i++) {
		if(edges[i].dependency == current->name) {
			sem_post(&edges[i].sem);
		}
	}

	cout << "Node " << current->name << " computed a value of " << comp << " after " << current->sleep_sec << " seconds." << endl;
}

int main(int argv, char* argc[]) {
	if(argv != 2) {
		cout << "Error: Please enter " << ((argv > 1) ? "only " : "") << "one argument on the command line." << endl;
		return 1;
	}
	ifstream myfile(argc[1]);
	if (!myfile.is_open()) {
		cout << "Unable to open file " << argc[1] << "." << endl;
		return 1;
	}

	string line;
	int i;
	char* input, token;
	while(getline(myfile, line)) {
		i = 0;
		char* input = strndup(line.c_str(), 50);
		char* token = strtok(input, " ");
		node new_node;
		while (token != NULL) {
			if(token[0] == '=') i=4;
			switch(i) {
			case 0: if(strlen(token) > 1 || token[0] < 'A' || token[0] > 'Z') {
						cout << "File format error: Each line must start with a single capital letter." << endl;
						return 1;
					} else new_node.name = token[0];
					break;

			case 1: if(!isInteger(token)) {
						cout << "File format error: The second argument on each line must be a number." << endl;
						return 1;
					} else {
						string lol(token);
						new_node.expr.push_back(lol);
					}
					break;

			case 2: if(!isInteger(token)) {
						cout << "File format error: The third argument on each line must be a number." << endl;
						return 1;
					} else new_node.sleep_sec = atoi(token);
					break;

			case 3: if(strlen(token) > 1 || token[0] < 'A' || token[0] > 'Z') {
						cout << "File format error: The fourth and later arguments on each line must all be single capital letters." << endl;
						return 1;
					} else {
						edge new_edge;
						new_edge.dependency = token[0];
						new_edge.result = new_node.name;
						sem_init(&new_edge.sem, 0, 0);
						edges.push_back(new_edge);
						i--; // keep state at 3
					}
					break;
			case 4: new_node.expr.pop_back(); // ignore the value from before
					new_node.needsEval = true;
					break;
			default:string lol(token);
					new_node.expr.push_back(lol);
			}
			i++;
			token = strtok(NULL, " ");
		}
		nodes.push_back(new_node);
	}

	/*for(int i = 0; i < edges.size(); i++) {
		cout << edges[i].result << " depends on " << edges[i].dependency << endl;
	}*/
	/*for(int i = 0; i < nodes.back().expr.size(); i++) {
		cout << nodes.back().expr[i] << endl;
	}*/

	sem_init(&mutex, 0, 1);

	struct timeval b;
	struct timeval a;
	gettimeofday(&b, NULL);

	for(int i = 0; i < nodes.size(); i++) {
		void *compute(void*);
		pthread_t id;
		pthreads.push_back(id);
		if(pthread_create(&pthreads[i], NULL, compute, (void*)&nodes[i])) {
			perror("pthread_create");
			return 1;
		}
	}

	for(int i = 0; i < pthreads.size(); i++) {
		pthread_join(pthreads[i], NULL);
	}

	gettimeofday(&a, NULL);

	int time = a.tv_sec - b.tv_sec;
	cout << "Total computation resulted in a value of " << globalValue << " after " << time << " seconds." << endl;

	for(int i = 0; i < edges.size(); i++) {
		sem_destroy(&edges[i].sem);
	}

	sem_destroy(&mutex);
	return 0;
}

#include <iostream>           // std::cout
#include <thread>             // std::thread
#include <chrono>             // std::chrono::seconds
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status

using namespace std;
using namespace std::chrono_literals;

const chrono::milliseconds makeTime[4] = { 50ms, 70ms, 90ms, 110ms };
const chrono::milliseconds assembleTime[4] = { 80ms, 100ms, 120ms, 140ms };
const chrono::nanoseconds moveTime1[4] = { 20ns, 30ns, 40ns, 50ns };
const chrono::nanoseconds moveTime2[4] = { 20ns, 30ns, 40ns, 50ns };
const chrono::nanoseconds discardTime1[4] = { 20ns, 30ns, 40ns, 50ns };
const chrono::nanoseconds discardTime2[4] = { 20ns, 30ns, 40ns, 50ns };
const chrono::milliseconds waitTime1 = 600ms;
const chrono::milliseconds waitTime2 = 1000ms;

const int bufferSize[4] = { 6, 5, 4, 3 };
int buffer[4] = { 0, 0, 0, 0 };
int finished = 0;
int discarded = 0;


void PartWorker(int ID) {
	 
}

void ProductWorker(int ID) {

}


int main() {
	const int m = 20, n = 16; //m: number of Part Workers
	//n: number of Product Workers
	//m>n
	thread partW[m];
	thread prodW[n];
	for (int i = 0; i < n; i++) {
		partW[i] = thread(PartWorker, i);
		prodW[i] = thread(ProductWorker, i);
	}
	for (int i = n; i < m; i++) {
		partW[i] = thread(PartWorker, i);
	}
	/* Join the threads to the main threads */
	for (int i = 0; i < n; i++) {
		partW[i].join();
		prodW[i].join();
	}
	for (int i = n; i < m; i++) {
		partW[i].join();
	}
	cout << "Finish!" << endl;
	return 0;
}
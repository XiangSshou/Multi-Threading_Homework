#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

using namespace std;
using namespace std::chrono_literals;

// I use more than needed for further changes
const chrono::milliseconds makeTime[4] = { 50ms, 70ms, 90ms, 110ms };
const chrono::milliseconds assembleTime[4] = { 80ms, 100ms, 120ms, 140ms };
const chrono::milliseconds moveTime1[4] = { 20ms, 30ms, 40ms, 50ms };
const chrono::milliseconds moveTime2[4] = { 20ms, 30ms, 40ms, 50ms };
const chrono::milliseconds discardTime1[4] = { 20ms, 30ms, 40ms, 50ms };
const chrono::milliseconds discardTime2[4] = { 20ms, 30ms, 40ms, 50ms };
const chrono::milliseconds waitTime1 = 3000ms;
const chrono::milliseconds waitTime2 = 6000ms;

const int bufferSize[4] = { 6, 5, 4, 3 };
int buffer[4] = { 0, 0, 0, 0 };
int finished = 0;
// the number of workers are set here
const int numberOfPartWorkers = 20;
const int numberOfProductWorkers = 16;
int workerCannotStop = numberOfPartWorkers + numberOfProductWorkers;

const char* filename = "log.txt";
ofstream file;

condition_variable cvPart;
condition_variable cvProduct;
mutex cv_m;

chrono::system_clock::time_point start;

void PartWorker(int ID) {
	srand((unsigned)ID);
	int iteration = 0, finishedIteration = 0;
	while (workerCannotStop > 0) {
		// generate a load order
		int loadOrder[4] = { 0, 0, 0, 0 };
		chrono::milliseconds totalMakeTime = 0ms;
		chrono::milliseconds totalMoveTime = 0ms;
		for (int i = 0; i < 4; ++i) {
			int j = rand() % 4;
			++loadOrder[j];
			totalMakeTime += makeTime[j];
			totalMoveTime += moveTime1[j];
		}

		// sleep to make parts
		this_thread::sleep_for(totalMakeTime);
		// sleep to move parts
		this_thread::sleep_for(totalMoveTime);

		// access the buffer
		unique_lock<std::mutex> lk(cv_m);

		// use the buffer for the first time;
		file << "Current Time: " << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start).count() << " us" <<endl;
		file << "Part Worker ID: " << ID << endl;
		file << "Iteration: " << iteration << endl;
		file << "Status: New Load Order" << endl;
		file << "Accumulated Wait Time: 0us" << endl;
		file << "Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
		file << "Load Order: (" << loadOrder[0] << "," << loadOrder[1] << "," << loadOrder[2] << "," << loadOrder[3] << ")" << endl;
		// load the parts
		for (int i = 0; i < 4; i++) {
			while (loadOrder[i] > 0 && bufferSize[i] > buffer[i]) {
				--loadOrder[i];
				++buffer[i];
			}
		}
		file << "Updated Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
		file << "Updated Load Order: (" << loadOrder[0] << "," << loadOrder[1] << "," << loadOrder[2] << "," << loadOrder[3] << ")" << endl;
		file << endl;
		lk.unlock();
		// the number of left parts
		int left = 0;
		for (auto part : loadOrder)left += part;

		// if there are some parts left start to wait
		chrono::system_clock::time_point startWaiting = chrono::system_clock::now();
		while (left != 0) {
			lk.lock();
			if (cvPart.wait_until(lk, startWaiting + waitTime1) != std::cv_status::timeout) {
				file << "Current Time: " << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start).count() << " us" << endl;
				file << "Part Worker ID: " << ID << endl;
				file << "Iteration: " << iteration << endl;
				file << "Status: Wakeup-Notified" << endl;
				file << "Accumulated Wait Time: " << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - startWaiting).count() << "us" << endl;
				file << "Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
				file << "Load Order: (" << loadOrder[0] << "," << loadOrder[1] << "," << loadOrder[2] << "," << loadOrder[3] << ")" << endl;
				for (int i = 0; i < 4; i++) {
					while (loadOrder[i] > 0 && bufferSize[i] > buffer[i]) {
						--loadOrder[i];
						--left;
						++buffer[i];
					}
				}
				file << "Updated Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
				file << "Updated Load Order: (" << loadOrder[0] << "," << loadOrder[1] << "," << loadOrder[2] << "," << loadOrder[3] << ")" << endl;
				file << endl;
				lk.unlock();
			}
			else {
				file << "Current Time: " << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start).count() << " us" << endl;
				file << "Part Worker ID: " << ID << endl;
				file << "Iteration: " << iteration << endl;
				file << "Status: Wakeup-Timeout" << endl;
				file << "Accumulated Wait Time: " << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - startWaiting).count() << "us" << endl;
				file << "Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
				file << "Load Order: (" << loadOrder[0] << "," << loadOrder[1] << "," << loadOrder[2] << "," << loadOrder[3] << ")" << endl;
				for (int i = 0; i < 4; i++) {
					while (loadOrder[i] > 0 && bufferSize[i] > buffer[i]) {
						--loadOrder[i];
						--left;
						++buffer[i];
					}
				}
				file << "Updated Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
				file << "Updated Load Order: (" << loadOrder[0] << "," << loadOrder[1] << "," << loadOrder[2] << "," << loadOrder[3] << ")" << endl;
				file << endl;
				lk.unlock();
				break;
			}
		}
		cvProduct.notify_one();

		if (left != 0) {
			chrono::milliseconds totalDiscardTime = 0ms;
			for (int i = 0; i < 4; i++) {
				while (loadOrder[i] > 0 && bufferSize[i] > buffer[i]) {
					--loadOrder[i];
					--left;
					totalDiscardTime += discardTime1[i];
				}
			}
			this_thread::sleep_for(totalDiscardTime);
		}
		else {
			++finishedIteration;
		}

		++iteration;
		if (finishedIteration == 5) --workerCannotStop;
	}
}

void ProductWorker(int ID) {
	srand((unsigned)ID);
	int iteration = 0, finishedIteration = 0;
	while (workerCannotStop > 0) {
		// generate a pickup order
		int pickupOrder[4] = { 1, 1, 1, 1 };
		int empty = rand() % 4;
		--pickupOrder[empty];
		chrono::milliseconds totalAssembleTime = 0ms, totalDiscardTime = 0ms;
		for (int i = 0; i < 2; ++i) {
			int j = rand() % 3;
			if (j >= empty)++j;
			++pickupOrder[j];
			totalAssembleTime += assembleTime[j];
			totalDiscardTime += discardTime2[j];
		}

		// access the buffer
		unique_lock<std::mutex> lk(cv_m);

		// use the buffer for the first time;
		file << "Current Time: " << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start).count() << " us" << endl;
		file << "Product Worker ID: " << ID << endl;
		file << "Iteration: " << iteration << endl;
		file << "Status: New Load Order" << endl;
		file << "Accumulated Wait Time: 0 us" << endl;
		file << "Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
		file << "Pickup Order: (" << pickupOrder[0] << "," << pickupOrder[1] << "," << pickupOrder[2] << "," << pickupOrder[3] << ")" << endl;
		chrono::milliseconds totalMoveTime1 = 0ms;
		for (int i = 0; i < 4; i++) {
			while (pickupOrder[i] > 0 && buffer[i] > 0) {
				--pickupOrder[i];
				--buffer[i];
				totalMoveTime1 += moveTime1[i];
			}
		}
		this_thread::sleep_for(totalMoveTime1);
		file << "Updated Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
		file << "Updated Pickup Order: (" << pickupOrder[0] << "," << pickupOrder[1] << "," << pickupOrder[2] << "," << pickupOrder[3] << ")" << endl;
		file << endl;
		lk.unlock();
		// the number of left parts
		int left = 0;
		for (auto part : pickupOrder)left += part;

		// if there are some parts left start to wait
		chrono::system_clock::time_point startWaiting = chrono::system_clock::now();
		while (left != 0) {
			lk.lock();
			if (cvProduct.wait_until(lk, startWaiting + waitTime2) != std::cv_status::timeout) {
				file << "Current Time: " << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start).count() << " us" << endl;
				file << "Product Worker ID: " << ID << endl;
				file << "Iteration: " << iteration << endl;
				file << "Status: Wakeup-Notified" << endl;
				file << "Accumulated Wait Time: " << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - startWaiting).count() << "us" << endl;
				file << "Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
				file << "Pickup Order: (" << pickupOrder[0] << "," << pickupOrder[1] << "," << pickupOrder[2] << "," << pickupOrder[3] << ")" << endl;
				chrono::milliseconds totalMoveTime2 = 0ms;
				for (int i = 0; i < 4; i++) {
					while (pickupOrder[i] > 0 && buffer[i] > 0) {
						--pickupOrder[i];
						--left;
						--buffer[i];
						totalMoveTime2 += moveTime1[i];
					}
				}
				this_thread::sleep_for(totalMoveTime2);
				file << "Updated Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
				file << "Updated Pickup Order: (" << pickupOrder[0] << "," << pickupOrder[1] << "," << pickupOrder[2] << "," << pickupOrder[3] << ")" << endl;
				file << endl;
				lk.unlock();
			}
			else {
				file << "Current Time: " << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start).count() << " us" << endl;
				file << "Product Worker ID: " << ID << endl;
				file << "Iteration: " << iteration << endl;
				file << "Status: Wakeup-Timeout" << endl;
				file << "Accumulated Wait Time: " << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - startWaiting).count() << "us" << endl;
				file << "Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
				file << "Pickup Order: (" << pickupOrder[0] << "," << pickupOrder[1] << "," << pickupOrder[2] << "," << pickupOrder[3] << ")" << endl;
				chrono::milliseconds totalMoveTime2 = 0ms;
				for (int i = 0; i < 4; i++) {
					while (pickupOrder[i] > 0 && buffer[i] > 0) {
						--pickupOrder[i];
						--left;
						--buffer[i];
						totalMoveTime2 += moveTime1[i];
					}
				}
				this_thread::sleep_for(totalMoveTime2);
				file << "Updated Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
				file << "Updated Pickup Order: (" << pickupOrder[0] << "," << pickupOrder[1] << "," << pickupOrder[2] << "," << pickupOrder[3] << ")" << endl;
				file << "Total Completed Products: " << finished << endl;
				file << endl;
				lk.unlock();
				break;
			}
		}
		cvPart.notify_one();

		if (left != 0) {
			for (int i = 0; i < 4; i++) {
				while (pickupOrder[i] > 0 && bufferSize[i] > buffer[i]) {
					--pickupOrder[i];
					--left;
					totalDiscardTime -= discardTime1[i];
				}
			}
			this_thread::sleep_for(totalDiscardTime);
		}
		else {
			this_thread::sleep_for(totalAssembleTime);
			++finishedIteration;
			++finished;
		}

		++iteration;
		if (finishedIteration == 5) --workerCannotStop;
	}
}


int main() {
	file.open(filename, fstream::trunc | fstream::out);

	const int m = numberOfPartWorkers, n = numberOfProductWorkers;
	//m: number of Part Workers
	//n: number of Product Workers
	//m>n
	start = chrono::system_clock::now();

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

	file.close();
	cout << "Finish!" << endl;
	return 0;
}
#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

using namespace std;

vector<int> data;
atomic<bool> dataReady(false);

void read() {
	while (!dataReady.load()) {
		this_thread::sleep_for(chrono::milliseconds(1));
	}
	cout << data[0] << endl;
}

void write() {
	data.push_back(1);
	dataReady = true;
}

int main() {
	thread readThread(read);
	thread writeThread(write);
	readThread.join();
	writeThread.join();
	return 0;
}

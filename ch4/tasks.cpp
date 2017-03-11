#include <iostream>
#include <future>
#include <thread>

using namespace std;

int main() {
	packaged_task<void()> task([](){
		int total = 0;
		for (int i = 0; i < 10000; ++i) {
			total += i;
		}
		cout << total << endl;
		// return total;
	});
	auto taskFuture = task.get_future();
	thread thd(move(task));
	thd.detach();
	taskFuture.wait();
	return 0;
}
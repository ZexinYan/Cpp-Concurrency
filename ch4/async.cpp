#include <iostream>
#include <future>
#include <mutex>

using namespace std;

int total{0};
mutex m_mutex;

int addUp() {
	lock_guard<mutex> lock(m_mutex);
	for (int i = 0; i < 10000; ++i) {
		total += i;
	}
	return total;
}

int main() {
	auto thd1 = async(addUp);
	cout << thd1.get() << endl;
	return 0;
}

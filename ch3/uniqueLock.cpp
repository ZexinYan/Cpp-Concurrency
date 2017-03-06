#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

struct Data {
	int value;
};

class X {
private:
	Data m_value;
	mutex m_mutex;
public:
	X(Data& org) : m_value(org) {}
	friend void swap(X& lhs, X& rhs) {
		if (&lhs == &rhs) {
			return;
		}
		unique_lock<mutex> lock_a(lhs.m_mutex, defer_lock);
		unique_lock<mutex> lock_b(lhs.m_mutex, defer_lock);
		lock(lock_a, lock_b);
		swap(lhs.m_value, rhs.m_value);
	}
};

unique_lock<mutex> get_lock() {
	extern mutex someMutex;
	unique_lock<mutex> lk(someMutex);
	prepareData();
	return lk;
}

void processData() {
	unique_lock<mutex> lk(get_lock());
	doSomething();
}

#include <mutex>
#include <thread>
#include <iostream>
using namespace std;

class HierarchicalMutex {
private:
	mutex m_internalMuetx;
	unsigned long const m_selfMutexValue;
	unsigned long m_previousMutex;
	static thread_local unsigned long this_threadMutexValue;

	void checkForViolation() {
		if (this_threadMutexValue <= m_selfMutexValue) {
			throw logic_error("mutex Hierarchical violated!");
		}
	}

	void updateMutexValue() {
		m_previousMutex = this_threadMutexValue;
		this_threadMutexValue = m_selfMutexValue;
	}
public:
	explicit HierarchicalMutex(unsigned long mutexValue) : m_selfMutexValue(mutexValue), m_previousMutex(0) {}
	void lock() {
		checkForViolation();
		m_internalMuetx.lock();
		updateMutexValue();
	}

	bool tryLock() {
		checkForViolation();
		if (!m_internalMuetx.try_lock()) {
			return false;
		}
		updateMutexValue();
		return true;
	}

	void unlock() {
		this_threadMutexValue = m_previousMutex;
		updateMutexValue();
	}
};

thread_local unsigned long HierarchicalMutex::this_threadMutexValue = 100000000;

HierarchicalMutex highLevelMutex(10000);
HierarchicalMutex lowLevelMutex(5000);

void doLowLevelStuff() {
	cout << "Do low level stuff!" << endl;
}

void lowLevelFunc() {
	lock_guard<HierarchicalMutex> lowLevelLock(lowLevelMutex);
	doLowLevelStuff();
}

void doHighLevelStuff() {
	cout << "Do high level stuff!" << endl;
}

void highLevelFunc() {
	lock_guard<HierarchicalMutex> highLevelLock(highLevelMutex);
	doHighLevelStuff();
}

void highThread() {
	highLevelFunc();
}

void lowThread() {
	lowLevelFunc();
}

void wrongThread() {
	lowLevelFunc();
	highLevelFunc();
}

int main() {
	// try {
	// 	thread wrong(wrongThread);
	// } catch (...) {
	// 	// cout << "Wrong" << endl;
	// }
	thread high(highThread);
	thread low(lowThread);
	low.join();
	high.join();
	return 0;
}

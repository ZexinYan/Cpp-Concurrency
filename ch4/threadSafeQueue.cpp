#ifndef THREADSAFEQUEUE
#define THREADSAFEQUEUE

#include <memory> // for shared_ptr
#include <mutex> // for mutex
#include <condition_variable>
#include <queue>

using namespace std;

template <typename value_type>
class ThreadSafeQueue {
private:
	mutex m_mutex;
	queue<value_type> m_data;
	condition_variable m_conditionVar;

public:
	ThreadSafeQueue() {}
	
	ThreadSafeQueue(const ThreadSafeQueue& other) {
		lock_guard<mutex> lock(other.m_mutex);
		m_data = other.m_data;
	}

	ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

	void push(value_type value) {
		lock_guard<mutex> lock(m_mutex);
		m_data.push(value);
		m_conditionVar.notify_one();
	}

	bool try_pop(value_type& result) {
		lock_guard<mutex> lock(m_mutex);
		if (m_data.empty()) {
			return false;
		}
		result = m_data.front();
		m_data.pop();
		return true;
	}

	shared_ptr<value_type> try_pop() {
		lock_guard<mutex> lock(m_mutex);
		if (m_data.empty()) {
			return shared_ptr<value_type>();
		}
		shared_ptr<value_type> result(make_shared<value_type>(m_data.front()));
		m_data.pop();
		return result;
	}

	void wait_and_pop(value_type& result) {
		unique_lock<mutex> uniqueLock(m_mutex);
		m_conditionVar.wait(uniqueLock, [this]() {
			return !this->m_data.empty();
		});
		result = m_data.front();
		m_data.pop();
		uniqueLock.unlock();
	}

	shared_ptr<value_type> wait_and_pop() {
		unique_lock<mutex> uniqueLock(m_mutex);
		m_conditionVar.wait(m_mutex, [this]() {
			return !this->m_data.empty();
		});
		value_type result = m_data.front();
		m_data.pop();
		uniqueLock.unlock();
		return shared_ptr<value_type>(make_shared<value_type>(result));
	}

	bool empty() const {
		lock_guard<mutex> lock(m_mutex);
		return m_data.empty();
	}
};

#endif

#include <thread>
#include <iostream>

int main() {
	ThreadSafeQueue<int> threadSafeQueue;

	thread processThd([&]() {
		int value;
		if (threadSafeQueue.try_pop(value)) {
			cout << "Try pop: " << value << endl;
		}
		threadSafeQueue.wait_and_pop(value);
		cout << "Pop value : " << value << endl;
	});

	thread pushThd([&] () {
		threadSafeQueue.push(10);
		threadSafeQueue.push(20);
	});

	processThd.join();
	pushThd.join();
	return 0;
}

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

template <typename value_type>
class ThreadSafe_queue {
private:
	queue<value_type> data;
	condition_variable m_cv;
	mutable mutex m_mutex;

public:
	ThreadSafe_queue() {}
	ThreadSafe_queue(const ThreadSafe_queue& other) {
		lock_guard<mutex> lock(other.m_mutex);
		data = other.data;
	}
	ThreadSafe_queue& operator=(const ThreadSafe_queue&) = delete;

	void push(value_type value) {
		lock_guard<mutex> lock(m_mutex);
		data.push(value);
		m_cv.notify_one();
	}

	void waitPop(value_type& result) {
		unique_lock<mutex> lock(m_mutex);
		m_cv.wait(lock, [&]() {
			cout << result << "waiting..." << endl;
			return !this->data.empty();
		});
		result = move(data.front());
		data.pop();
	}

	shared_ptr<value_type> waitPop() {
		unique_lock<mutex> lock(m_mutex);
		m_cv.wait(lock, [this]() {
			return !this->data.empty();
		});
		const shared_ptr<value_type> res = make_shared<value_type>(move(data.front()));
		data.pop();
		return res;
	}

	bool tryPop(value_type& result) {
		lock_guard<mutex> lock(m_mutex);
		if (data.empty()) {
			return false;
		}
		result = move(data.front());
		data.pop();
		return true;
	}

	shared_ptr<value_type> tryPop() {
		lock_guard<mutex> lock(m_mutex);
		if (data.empty()) {
			return shared_ptr<value_type>();
		}
		shared_ptr<value_type> res = make_shared<value_type>(move(data.front()));
		data.pop();
		return res;
	}

	bool empty() {
		lock_guard<mutex> lock(m_mutex);
		return data.empty();
	}
};

int main() {
	ThreadSafe_queue<int> t_queue;
	thread b([&]() {
		int value = 1;
		t_queue.waitPop(value);
		cout << value << endl;
	});
	thread c([&]() {
		int value = 2;
		t_queue.waitPop(value);
		cout << value << endl;
	});
	thread a(&ThreadSafe_queue<int>::push, &t_queue, 10);
	b.join();
	a.join();
	c.join();
	return 0;
}

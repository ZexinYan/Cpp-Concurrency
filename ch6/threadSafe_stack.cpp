#include <thread>
#include <mutex>
#include <stack>
#include <iostream>
#include <exception>

using namespace std;

struct empty_stack: exception {
	const char* what() const throw();
};

template <typename value_type>
class ThreadSafe_stack {
private:
	stack<value_type> data;
	mutable mutex m_mutex;

public:
	ThreadSafe_stack() {}
	ThreadSafe_stack(const ThreadSafe_stack& other) {
		lock_guard<mutex> lock(other.m_mutex);
		data = other.data;
	}
	ThreadSafe_stack& operator=(const ThreadSafe_stack& other) = delete;

	void push(value_type value) {
		lock_guard<mutex> lock(m_mutex);
		data.push(move(value));
	}

	void pop(value_type& result) {
		lock_guard<mutex> lock(m_mutex);
		if (data.empty()) {
			// throw empty_stack();
		}
		result = move(data.top());
		data.pop();
	}

	shared_ptr<value_type> pop() {
		lock_guard<mutex> lock(m_mutex);
		if (data.empty()) {
			// throw empty_stack();
		}
		const shared_ptr<value_type> res = make_shared<value_type>(move(data.top()));
		data.pop();
		return res;
	}

	bool empty() {
		lock_guard<mutex> lock(m_mutex);
		return data.empty();
	}
};

int main() {
	ThreadSafe_stack<int> t_stack;
	thread a(&ThreadSafe_stack<int>::push, &t_stack, 10);
	thread b([&]() {
		int value;
		t_stack.pop(value);
		cout << value << endl;
	});
	a.join();
	b.join();
	return 0;
}

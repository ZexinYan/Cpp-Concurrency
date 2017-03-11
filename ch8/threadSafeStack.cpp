#include <exception>
#include <memory>
#include <mutex>
#include <stack>

using namespace std;
struct emptyStack : exception {
	const char* what() const throw();
};

template <typename valueType>
class threadSafeStack {
private:
	stack<valueType> m_stack;
	mutable mutex m_mutex;
public:
	threadSafeStack(){}
	threadSafeStack(const threadSafeStack& other) {
		lock_guard<mutex> lock(m_mutex);
		m_stack = other.data;
	}
	threadSafeStack& operator=(const threadSafeStack&) = delete;

	void push(valueType&& value) {
		lock_guard<mutex> lock(m_mutex);
		m_stack.push(value);
	}

	// void push(valueType&& value) {
	// 	lock_guard<mutex> lock(m_mutex);
	// 	m_stack.push(value);
	// }

	shared_ptr<valueType> pop() {
		lock_guard<mutex> lock(m_mutex);
		if (m_stack.empty()) {
			throw emptyStack();
		}
		shared_ptr<valueType> const res(make_shared<valueType>(m_stack.top()));
		m_stack.pop();
		return res;
	}

	void pop(valueType& value) {
		lock_guard<mutex> lock(m_mutex);
		if (m_stack.empty()) {
			throw emptyStack();
		}
		value = m_stack.top();
		m_stack.pop();
	}

	bool empty() const {
		lock_guard<mutex> lock(m_mutex);
		return m_stack.empty();
	}
};


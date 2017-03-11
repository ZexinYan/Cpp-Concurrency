#include <iostream>
#include <mutex>

using namespace std;

template <typename value_type>
class ThreadSafe_queue {
private:
	struct Node {
		shared_ptr<value_type> data;
		unique_ptr<Node> next;
	};
	mutex m_headMutex;
	mutex m_tailMutex;
	unique_ptr<Node> head;
	Node* tail;

	Node* getTail() {
		lock_guard<mutex> lock(m_tailMutex);
		return tail;
	}

	unique_ptr<Node> popHead() {
		lock_guard<mutex> lock(m_headMutex);
		if (head.get() == tail) {
			return nullptr;
		}
		auto oldHead = move(head);
		head = move(oldHead->next);
		return oldHead;
	}

public:
	ThreadSafe_queue():
		head(new Node), tail(head.get()) {}
	ThreadSafe_queue(const ThreadSafe_queue&) = delete;
	ThreadSafe_queue& operator=(const ThreadSafe_queue&) = delete;

	shared_ptr<value_type> tryPop() {
		auto oldHead = popHead();
		return oldHead ? oldHead->data : shared_ptr<value_type>();
	}

	void push(value_type result) {
		auto newData = make_shared<value_type>(move(result));
		unique_ptr<value_type> p(new Node);
		Node* const newTail = p.get();
		lock_guard<mutex> lock(m_tailMutex);
		tail->data = newData;
		tail->next = move(p);
		tail = newTail;
	}
};

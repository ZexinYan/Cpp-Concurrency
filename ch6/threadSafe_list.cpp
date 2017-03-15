#include <iostream>
#include <mutex>
#include <thread>
#include <memory>

using namespace std;

template <typename value_type>
class threadSafe_list {
private:
	struct Node {
		shared_ptr<value_type> data;
		unique_ptr<Node> next; // make sure that only one ptr will point to next.
		mutex m_mutex;
		Node() : next() {}
		Node(value_type const& value) : data(make_shared<value_type>(value)) {}
	};
	Node head;

public:
	threadSafe_list() {}
	~threadSafe_list() {
		remove_if([](Node const&) { return true; });
	}
	threadSafe_list(threadSafe_list const&) = delete;
	threadSafe_list& operator=(threadSafe_list const&) = delete;

	void push_front(value_type const& value) {
		unique_ptr<Node> new_node(new Node(value));
		lock_guard<mutex> head_lock(head.m_mutex);
		new_node->next = move(head.next);
		head.next = move(new_node);
	}

	template <typename Function>
	void for_each(Function func) {
		Node* current = &head;
		unique_lock<mutex> now_lock(head.m_mutex);
		while (Node* const next = current->next.get()) {
			unique_lock<mutex> next_lock(next->m_mutex);
			now_lock.unlock();
			func(*next->data);
			current = next;
			now_lock = move(next_lock);
		}
	}

	template <typename Predicate>
	shared_ptr<value_type> find_first_if(Predicate pred) {
		Node* current = &head;
		unique_lock<mutex> now_lock(head.m_mutex);
		while (Node* const next = current->next.get()) {
			unique_lock<mutex> next_lock(next->mutex);
			now_lock.unlock();
			if (pred(*next->data)) {
				return next->data;
			}
			current = next;
			now_lock = move(next_lock);
		}
		return shared_ptr<value_type>();
	}

	template <typename Predicate>
	void remove_if(Predicate pred) {
		Node* current = &head;
		unique_lock<mutex> now_lock(head.m_mutex);
		while (Node* const next = current->next.get()) {
			unique_lock<mutex> next_lock(next->m_mutex);
			if (pred(*next->data)) {
				unique_ptr<Node> old_next = move(current->next);
				current->next = move(next->next);
				next_lock.unlock();
			} else {
				now_lock.unlock();
				current = next;
				now_lock = move(next_lock);
			}
		}
	}
};

int main() {
	threadSafe_list<int> list;
	thread a([&]() {
		list.push_front(10);
		list.push_front(20);
	});

	thread b([&]() {
		list.for_each([](int value) {
			cout << value << endl;
		});
	});
	a.join();
	b.join();
	return 0;
}

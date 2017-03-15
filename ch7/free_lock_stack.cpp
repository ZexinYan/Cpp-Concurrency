#include <iostream>
#include <atomic>
#include <memory>
using namespace std;

template <typename value_type>
class free_lock_stack {
private:
	class Node {
		shared_ptr<value_type> data;
		unique_ptr<Node> next;

		Node(value_type const& new_value) :
			data(make_shared<value_type>(new_value)) {}
	};
	atomic<Node*> head;

public:
	void push(value_type const& new_value) {
		Node* const new_node = new Node(new_value);
		new_node->next = head.load();
		while (!head.compare_exchange_weak(new_node->next, new_node));
	}

	shared_ptr<value_type> pop() {
		Node* old_head = head.load();
		while (old_head && !head.compare_exchange_weak(old_head, old_head->next));
		return old_head ? old_head->data : shared_ptr<value_type>();
	}
};

int main() {
	
}
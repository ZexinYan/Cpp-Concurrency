#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <atomic>
#include <future>
#include <memory>
#include <algorithm>
#include <mutex>
#include <stack>
using namespace std;

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

	void push(valueType value) {
		lock_guard<mutex> lock(m_mutex);
		m_stack.push(move(value));
	}

	shared_ptr<valueType> pop() {
		lock_guard<mutex> lock(m_mutex);
		if (m_stack.empty()) {
			cout << "empty" << endl;
			return shared_ptr<valueType>();
		}
		shared_ptr<valueType> const res(make_shared<valueType>(move(m_stack.top())));
		m_stack.pop();
		return res;
	}

	void pop(valueType& value) {
		lock_guard<mutex> lock(m_mutex);
		if (m_stack.empty()) {
			return;
			// throw emptyStack();
		}
		value = move(m_stack.top());
		m_stack.pop();
	}

	bool empty() const {
		lock_guard<mutex> lock(m_mutex);
		return m_stack.empty();
	}
};

template <typename value_type>
class Sorter {
public:
	struct chunkToSort {
		list<value_type> data;
		// for asynchronism
		promise<list<value_type>> Promise;
	};

	threadSafeStack<chunkToSort> chunks;
	vector<thread> threads;
	unsigned const maxThreadCount;
	atomic<bool> endOfData{false};
	
public:
	Sorter() :
			maxThreadCount(thread::hardware_concurrency() - 1) {}

	~Sorter() {
		endOfData = true;
		for (int i = 0; i < threads.size(); ++i) {
			threads[i].join();
		}
	}

	list<value_type> doSort(list<value_type>& chunkData) {
		if (chunkData.empty()) {
			return chunkData;
		}

		list<value_type> result;
		result.splice(result.begin(), chunkData, chunkData.begin());
		value_type const& pivot = *result.begin();

		typename list<value_type>::iterator dividePoint = partition(chunkData.begin(), chunkData.end(), [&pivot](const value_type& value) {
			return pivot > value;
		});

		chunkToSort newLowerChunk;
		newLowerChunk.data.splice(newLowerChunk.data.end(), chunkData, chunkData.begin(), dividePoint);
		
		auto newLowerFuture = newLowerChunk.Promise.get_future();
		chunks.push(move(newLowerChunk));

		// allocate thread to process.
		if (threads.size() < maxThreadCount) {
			threads.push_back(thread(&Sorter<value_type>::sortThread, this));
		}

		// recursion.
		list<value_type> newHigher(doSort(chunkData));

		result.splice(result.end(), newHigher);

		// when this thread are waiting, it can help by processing other data.
		while (newLowerFuture.wait_for(chrono::seconds(0)) != future_status::ready) {
			trySortChunk();
		}

		result.splice(result.begin(), newLowerFuture.get());

		return result;
	}
	
	void sortThread() {
		while (!endOfData) {
			trySortChunk();
			// suspent this.
			// the os will schedule this thread.
			this_thread::yield();
		}
	}

	void trySortChunk() {
		shared_ptr<chunkToSort> chunk = chunks.pop();
		if (chunk) {
			sortChunk(chunk);
		}
	}

	void sortChunk(const shared_ptr<chunkToSort>& chunk) {
		chunk->Promise.set_value(doSort(chunk->data));
	}

};

template <typename value_type>
list<value_type> parallelQuickSort(list<value_type> input) {
	if (input.empty()) {
		return input;
	}
	Sorter<value_type> s;
	return s.doSort(input);
}

int main() {
	list<int> input{1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 7,1, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 71, 2, 1, 7};
	input = parallelQuickSort(input);
	for_each(input.begin(), input.end(), [](int value) {
		cout << value << " ";
	});
	cout << endl;
	return 0;
}

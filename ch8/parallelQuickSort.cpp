#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <atomic>
#include <future>
#include <memory>
#include <algorithm>

#include "threadSafeStack.cpp"

using namespace std;

template <typename value_type>
class Sorter {
public:
	struct chunkToSort {
		list<value_type> data;
		promise<list<value_type>> Promise;
	};

	threadSafeStack<chunkToSort> chunks;
	vector<thread> threads;
	unsigned const maxThreadCount{thread::hardware_concurrency() - 1};
	atomic<bool> endOfData{false};
	
public:
	Sorter() {
		// maxThreadCount(thread::har/dware_concurrency() - 1);
	}

	~Sorter() {
		endOfData = true;
		for (int i = 0; i < threads.size(); ++i) {
			threads[i].join();
		}
	}

	void trySortChunk() {
		 shared_ptr<chunkToSort> chunk = chunks.pop();
		if (chunk) {
			sortChunk(chunk);
		}
	}

	list<value_type> doSort(list<value_type>& chunkData) {
		if (chunkData.empty()) {
			return chunkData;
		}
		list<value_type> result;
		result.splice(result.end(), chunkData, chunkData.begin());
		value_type pivot = *result.begin();

		typename list<value_type>::iterator dividePoint = partition(chunkData.begin(), chunkData.end(), [&pivot](const value_type& value) {
			return pivot < value;
		});

		chunkToSort newLowerChunk;
		newLowerChunk.data.splice(newLowerChunk.data.end(), chunkData, chunkData.begin(), dividePoint);
		
		auto newLowerFuture = newLowerChunk.Promise.get_future();
		chunks.push(move(newLowerChunk));

		// if (threads.size() < maxThreadCount) {
		// 	threads.push_back(thread(&Sorter<value_type>::sortThread, this));
		// }

		// list<value_type> newHigher(doSort(chunkData));

		// result.splice(result.end(), newHigher);

		// while (newLowerFuture.wait_for(chrono::seconds(0)) != future_status::ready) {
		// 	trySortChunk();
		// }

		// result.splice(result.begin(), newLowerFuture.get());

		return result;
	}

	void sortChunk(const shared_ptr<chunkToSort>& chunk) {
		chunk->Promise.set_value(doSort(chunk->data));
	}

	void sortThread() {
		while (!endOfData) {
			trySortChunk();
			this_thread::yield();
		}
	}
};

template <typename value_type>
list<value_type> parallelQuickSort(list<value_type>  input) {
	if (input.empty()) {
		return input;
	}
	Sorter<value_type> s;
	return s.doSort(input);
}

int main() {
	list<int> input{1, 2, 1, 7, 2,1,2,4,5,6,7,2,2,4,5,6,2,1,5,8,3,50, 4,5,2,1};
	parallelQuickSort(input);
	for_each(input.begin(), input.end(), [](int value) {
		cout << value << " ";
	});
	cout << endl;
	return 0;
}

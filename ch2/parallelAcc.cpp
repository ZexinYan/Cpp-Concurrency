#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <numeric>
using namespace std;

template <typename Iterator, typename value>
class accumlateBlock {
public:
	void operator()(Iterator first, Iterator last, value& result) {
		result = accumulate(first, last, result);
	}
};

template <typename Iterator, typename value>
value parallelAccumulate(Iterator first, Iterator last, value init) {
	unsigned long const length = distance(first, last);
	if (length == 0) {
		return init;
	}

	unsigned long const minPerThread = 25;
	unsigned long const maxThreads = (length + minPerThread - 1) / minPerThread;
	unsigned long const hardwareThreads = thread::hardware_concurrency();
	unsigned long const threadSize = min(hardwareThreads != 0 ? hardwareThreads : 2, maxThreads);
	unsigned long const blockSize = length / threadSize;

	vector<value> result(threadSize);
	vector<thread> threads(threadSize - 1);

	Iterator blockStart = first;

	for (unsigned long i = 0; i < (threadSize - 1); ++i) {
		Iterator blockEnd = blockStart;
		advance(blockEnd, blockSize);
		threads[i] = thread(accumlateBlock<Iterator, value>(), blockStart, blockEnd, ref(result[i]));
		blockStart = blockEnd;
	}

	accumlateBlock<Iterator, value>()(blockStart, last, result[threadSize - 1]);

	for_each(threads.begin(), threads.end(), [](thread& thd) {
		thd.join();
	});

	return accumulate(result.begin(), result.end(), init);
}

int main() {
	vector<int> vec(10000000, 1);
	int result = parallelAccumulate(vec.begin(), vec.end(), 0);
	cout << result << endl;
	return 0;
}

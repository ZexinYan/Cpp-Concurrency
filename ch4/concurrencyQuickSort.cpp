#include <iostream>
#include <list>
#include <future>
#include <algorithm>

using namespace std;

template <typename value_type>
list<value_type> parallelQuickSort(list<value_type> input) {
	if (input.empty()) {
		return list<value_type>();
	}
	list<value_type> result;
	result.splice(result.begin(), input, input.begin());
	auto pivot = *result.begin();
	auto dividePoint = partition(input.begin(), input.end(), [pivot](value_type const& value) {
		return value < pivot;
	});

	list<value_type> lowPart;
	lowPart.splice(lowPart.end(), input, input.begin(), dividePoint);

	auto newLowerFuture(async(&parallelQuickSort<value_type>, move(lowPart)));

	list<value_type> highPart(parallelQuickSort(move(input)));
	result.splice(result.end(), highPart);
	result.splice(result.begin(), newLowerFuture.get());
	return result;
}

int main() {
	list<int> values{4, 2, 3, 1, 7, 2, 3, 5, 8, 9, 2, 3, 10, 20, 28, 35, 15, 2};
	auto result = parallelQuickSort(values);
	for_each(result.begin(), result.end(), [](int value) {
		cout << value << " ";
	});
	cout << endl;
	return 0;
}

#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <numeric>
#include <algorithm>

using namespace std;

/**
 * For exceptional safety.
 */
class join_threads {
private:
	std::vector<thread>& m_threads;
public:
	join_threads(std::vector<thread>& t_thread) :
		m_threads(t_thread) {}
	/**
	 * If throwing exception before the threads are joined,
	 * it will automatically join to avoid threads become dangling.
	 */
	~join_threads() {
		for (int i = 0; i < m_threads.size(); ++i) {
			if (m_threads[i].joinable()) {
				m_threads[i].join();
			}
		}
	}
};

template <typename Iterator, typename T>
T parallel_accumulate(Iterator begin, Iterator end, T init) {
	unsigned long length = distance(begin, end);
	if (!length) {
		return init;
	}
	unsigned long const min_per_threads = 25;
	unsigned long const max_threads = (length + min_per_threads - 1) / min_per_threads;
	unsigned long const hardware_threads = thread::hardware_concurrency();
	unsigned long const num_threads = min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

	std::vector<future<T>> futures(num_threads - 1);
	std::vector<thread> threads(num_threads - 1);
	join_threads joiner(threads);

	unsigned long const block_size = length / num_threads;
	Iterator block_start = begin;
	for (int i = 0; i < num_threads - 1; ++i) {
		Iterator block_end = block_start;
		advance(block_end, block_size);
		packaged_task<T(Iterator, Iterator)> task([](Iterator begin, Iterator end) -> T {
			return accumulate(begin, end, T());
		});
		futures[i] = task.get_future();
		threads[i] = thread(move(task), block_start, block_end);
		block_start = block_end;
	}
	T last_result = accumulate_block<Iterator, T>()(block_start, end);
	T result = init;
	for (int i = 0; i < num_threads - 1; ++i) {
		result += futures[i].get();
	}
	result += last_result;
	return result;
}


template <typename Iterator, typename T>
Iterator parallel_find(Iterator begin, Iterator end, T match) {
	struct find_element {
		void operator()(Iterator begin, Iterator end,
						T match, promise<Iterator>* result, atomic<bool>* done_flag) {
			try {
				for (; begin != end && !done_flag->load(); begin++) {
					if (*begin == match) {
						result->set_value(begin);
						done_flag->store(true);
						return;
					}
				}
			} catch (...) {
				/**
				 * If the exception has been thrown,
				 * the func will terminiate.
				 */
				try {
					result->set_exception(current_exception());
					done_flag->store(true);
				} catch (...) {}
			}
		}
	};

	unsigned long const length = distance(begin, end);
	if (length == 0) {
		return end;
	}
	unsigned long const min_per_threads = 25;
	unsigned long const max_threads = (length + min_per_threads - 1) / min_per_threads;
	unsigned long const hardware_threads = thread::hardware_concurrency();
	unsigned long const num_threads = min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads;

	promise<Iterator> result;
	atomic<bool> done_flag(false);
	std::vector<thread> threads(num_threads - 1);
	
	{
		join_threads joiner(threads);
		Iterator block_start = begin;
		for (int i = 0; i < num_threads - 1; ++i) {
			Iterator block_end = block_start;
			advance(block_end, block_size);
			threads[i] = thread(find_element(), block_start, block_end, match, &result, &done_flag);
			block_start = block_end;
		}
		find_element()(block_start, end, match, &result, &done_flag);
	}
	if (!done_flag.load()) {
		return end;
	}
	return result.get_future().get();
}

template <typename Iterator>
void parallel_partial_sum(Iterator begin, Iterator end) {
	typedef typename Iterator::value_type value_type;

	struct process_chunk {
		void operator()(Iterator begin, Iterator last, 
						future<value_type>* previous_end_value,
						promise<value_type>* end_value) {
			try {
				Iterator end = last;
				end++;
				partial_sum(begin, end, begin);
				if (previous_end_value) {
					/**
					 * get: will guarantee the the value will be got.
					 * If ann exception are thrown, it will be rethrown.
					 * And propagate all exceptions into the final chunk.
					 */
					value_type append = previous_end_value->get();
					*last += append;
					if (end_value) {
						end_value->set_value(*last);
					}
					for_each(begin, last, [append](value_type& item) {
						item += append;
					});
				} else if (end_value) {
					end_value->set_value(*last);
				}
			} catch (...) {
				if (end_value) {
					end_value->set_exception(current_exception());
				} else {
					throw;
				}
			}
		}
	};

	unsigned long const length = distance(begin, end);
	if (length == 0) {
		return;
	}
	unsigned long const min_per_threads = 25;
	unsigned long const max_threads = (length + min_per_threads - 1) / min_per_threads;
	unsigned long const hardware_threads = thread::hardware_concurrency();
	unsigned long const num_threads = min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads;
	std::vector<thread> threads(num_threads - 1);
	std::vector<promise<value_type>> end_values(num_threads - 1);
	std::vector<future<value_type>> previous_end_values;
	previous_end_values.reserve(num_threads - 1);
	join_threads joiner(threads);

	Iterator block_start = begin;
	for (int i = 0; i < num_threads - 1; ++i) {
		Iterator block_last = block_start;
		advance(block_last, block_size - 1);
		threads[i] = thread(process_chunk(), block_start, block_last,
							(i != 0) ? &previous_end_values[i - 1] : 0,
							&end_values[i]);
		block_start = block_last;
		block_start++;
		previous_end_values.push_back(end_values[i].get_future());
	}
	Iterator final_element = block_start;
	advance(final_element, distance(block_start, end) - 1);
	process_chunk()(block_start, final_element, (num_threads > 1) ? &previous_end_values.back() : 0, 0);
}

struct barrier {
	atomic<unsigned> count;
	atomic<unsigned> spaces;
	atomic<unsigned> generation;

	barrier(int _count): count(_count), spaces(_count), generation(0) {}

	void wait() {
		unsigned const myGeneration = generation.load();
		if (!--spaces) {
			spaces = count.load();
			generation++;
		} else {
			while (myGeneration == generation.load()) {
				this_thread::yield();
			}
		}
	}

	void done_waiting() {
		count--;
		if (!--spaces) {
			spaces = count.load();
			generation++;
		}
	}
};

// template <typename Iterator>
// void parallel_partial_sum(Iterator begin, Iterator end) {
// 	typedef typename Iterator::value_type value_type;

// 	struct process_element {
// 		void operator()(Iterator first, Iterator last,
// 						std::vector<value_type>& buffer,
// 						unsigned i, barrier& b) {
			
// 		}
// 	};
// }

int main() {
	std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 101, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 10,2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 101, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5,2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 101, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5,2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5, 6, 7, 8, 101, 2, 3, 4, 5, 6, 7, 8, 10,1, 2, 3, 4, 5};
	// cout << parallel_accumulate(v.begin(), v.end(), 0) << endl;
	// cout << *parallel_find(v.begin(), v.end(), 2) << endl;
	parallel_partial_sum(v.begin(), v.end());
	for (auto each : v) {
		cout << each << " ";
	}
	cout << endl;
	return 0;
}

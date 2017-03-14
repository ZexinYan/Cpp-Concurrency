#include <shared_mutex>
#include <mutex>
#include <thread>
#include <map>
#include <vector>
#include <list>

using namespace std;

template <typename Key, typename Value, typename Hash = hash<Key>>
class threadSafe_lookup_table {
private:
	class bucket_type {
	private:
		typedef pair<Key, Value> bucket_value;
		typedef list<bucket_value> bucket_data;
		typedef typename bucket_data::iterator bucket_iterator;

		bucket_data data;
		mutable shared_mutex m_mutex;

		bucket_iterator find_entry_for(Key const& key) const {
			return find_if(data.begin(), data.end(), [&](bucket_value& item) {
				return item->first == key;
			});
		}
	public:
		Value value_for(Key const& key, Value const& default_value) const {
			shared_lock<shared_mutex> lock(m_mutex);
			auto entry_iterator = find_entry_for(key);
			return entry_iterator == data.end() ? default_value : entry_iterator->second;
		}

		void add_or_update_mapping(Key const& key, Value const& newValue) {
			unique_lock<shared_mutex> lock(m_mutex);
			auto entry_iterator = find_entry_for(key);
			if (key == data.end()) {
				data.push_back(bucket_type(key, newValue));
			} else {
				entry_iterator->second = value;
			}
		}

		void remove_mapping(Key const& key) {
			unique_lock<shared_mutex> lock(m_mutex);
			auto entry_iterator = find_entry_for(key);
			if (entry_iterator != data.end()) {
				data.erase(entry_iterator);
			}
		}
	};

	vector<unique_ptr<bucket_type>> buckets;
	Hash hasher;
	bucket_type& get_bucket(Key& const key) {
		return *buckets[hasher(key) % buckets.size()];
	}

public:
	threadSafe_lookup_table(unsigned num_buckets = 19, Hash hasher_ = Hash())
		: buckets(num_buckets), hasher(hasher_) {
			for (int i = 0; i < num_buckets; ++i) {
				buckets[i].reset(new bucket_type);
			}
		}
	threadSafe_lookup_table& operator=(threadSafe_lookup_table const&) = delete;
	threadSafe_lookup_table(threadSafe_lookup_table const&) = delete;

	Value value_for(Key const& key, Value const& default_value) {
	 	return get_bucket(key).value_for(key, default_value);
	}

	void add_or_update_mapping(Key const& key, Value const& newValue) {
		get_bucket(key).add_or_update_mapping(key, newValue);
	}

	void remove_mapping(Key const& key) {
		get_bucket(key).remove_mapping(key);
	}
};

int main() {

}
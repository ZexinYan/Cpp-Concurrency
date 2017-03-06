#include <thread>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <stack>

using namespace std;

class ConditionVar {
private:
	stack<int> m_data;
	mutex m_mutex;
	condition_variable m_conditionVar;

public:
	ConditionVar() {}
	
	void dataPreparationThread() {
		int data = prepareData();
		lock_guard<mutex> lock(m_mutex);
		m_data.push(data);
		m_conditionVar.notify_one();
	}

	int prepareData() {
		int data;
		cout << "Input data" << endl;
		cin >> data;
		return data;
	}

	void process(int data) {
		cout << data << endl;
	}

	void dataProcessingThread() {
		unique_lock<mutex> uniqueLock(m_mutex);
		m_conditionVar.wait(uniqueLock, [this]() {
			return !this->m_data.empty();
		});
		int data = m_data.top();
		m_data.pop();
		uniqueLock.unlock();
		process(data);
	}
};

int main() {
	ConditionVar var;
	thread processThread(&ConditionVar::dataProcessingThread, &var);
	thread prepareThread(&ConditionVar::dataPreparationThread, &var);

	processThread.join();
	prepareThread.join();
	return 0;
}

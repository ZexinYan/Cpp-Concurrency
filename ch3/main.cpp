#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

class Data {
private:
	int value;
public:
	void doSomething() {
		cout << this->value << endl;
	}
};

class DataWrapper {
private:
	Data data;
	mutex m;
public:
	void processData(function<void (Data)> func) {
		lock_guard<mutex> guard(m);
		func(data);
	}
};

int main() {

}
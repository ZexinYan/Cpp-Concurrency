#include <iostream>
#include <thread>

using namespace std;


class GuardThread {
private:
	thread& t;
public:
	explicit GuardThread(thread& _t) : t(move(_t)) {
		if (!t.joinable()) {
			throw logic_error("No thread!");
		}
	}
	~GuardThread() {
		if (t.joinable()) {
			t.join();
		}
	}

	GuardThread(GuardThread const&) = delete;
	GuardThread& operator=(GuardThread const&) = delete;
};

void oop() {
	for (int i = 0; i < 100; ++i) {
		cout << i << endl;
	}
}

/*
The example for detach();
void editDocument(string const& filename) {
	openFile(filename);
	while (!doneEditing()) {
		userCommand cmd = getCommand();
		if (cmd.type == open_new_doc) {
			string const new_doc = get_filename();
			thread thd(editDocument, new_doc);
			thd.detach();
		} else {
			processCmd(cmd);
		}
	}
}
*/

class X {
public:
	int value{0};
	void doSomething() {
		cout << this->value << endl;
		cout << "DO something!" << endl;
	}
};

// thread constructor just copy the parameter
// if you wanna pass the reference to the func
// you just use ref();

void passRef(X& ref) {
	ref.value += 10;
}
int main() {
	X my_x;
	my_x.value = 10;
	thread thd(&X::doSomething, &my_x);
	thd.join();
	thread thd2(passRef, ref(my_x));
	thd2.join();
	cout << my_x.value << endl;
	return 0;
}

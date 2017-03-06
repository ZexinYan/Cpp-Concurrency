class Y {
private:
	int m_value;
	mutable mutex m_mutex;

	int getValue() const {
		lock_guard<mutex> lock(m_mutex);
		return m_value;
	}

public:
	Y(int org) : m_value(org) {}
	friend bool operator==(Y& lhs, Y& rhs) {
		if (&lhs == &rhs) {
			return true;
		}
		const int lvalue = lhs.getValue();
		const int rvalue = rhs.getValue();
		return lvalue == rvalue;
	}
};

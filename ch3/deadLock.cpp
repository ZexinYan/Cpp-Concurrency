class object {
private:
	int value;
};

void swap(object& lobj, object& robj);

class ThreadSafeObject {
private:
	mutex m_mutex;
	object obj;
public:
	friend void swap(ThreadSafeObject& lobj, ThreadSafeObject& robj) {
		if (&lobj == &robj) {
			return;
		}
		lock(lobj.m_mutex, robj.m_mutex); // solve the problem of more than two mutex exist.
		lock_guard<mutex> lobjLock(lobj.m_mutex, adopt_lock);
		lock_guard<mutex> robjLock(robj.m_mutex, adopt_lock);
		swap(lobj.obj, robj.obj);
	}
};

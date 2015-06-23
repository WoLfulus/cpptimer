#include <iostream>
#include <timer/timer.hpp>
#include <windows.h>

using namespace std;

int main(int argc, char* argv[])
{
	///
	/// Declares the manager
	///
	timer::manager mgr;

	///
	/// Intervals (runs forever)
	///
	mgr.interval([]() {
		cout << "Happens every 5 second" << endl;
	}, std::chrono::seconds(5));

	///
	/// Timeouts (once)
	///
	mgr.timeout([]() {
		cout << "Happens once after 10 seconds" << endl;
	}, std::chrono::seconds(10));

	///
	/// Repeat (N times)
	///
	mgr.repeat([]() {
		cout << "Happens 5 times every 3 seconds" << endl;
	}, std::chrono::seconds(3), 5);

	///
	/// 
	///
	auto t = mgr.repeat([]() {
		cout << "Schedule to happen 5 times every 3 second but will run only twice" << endl;
	}, std::chrono::seconds(3), 5);

	///
	/// Cancels the previous timer
	///
	mgr.timeout([&] () {
		cout << "Canceling timer" << t << endl;
		mgr.cancel(t);
	}, std::chrono::seconds(8));

	///
	/// Starts updates
	///
	mgr.start();
	mgr.wait();

	return 0;
}


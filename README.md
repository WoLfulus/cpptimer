cpptimer
============

Simple C++11 timers (interval, timeout, repeat).

In-process timers for periodic jobs that uses builder pattern for configuration. cpptimer lets you run any thing convertible to a **std::function<void()>** periodically at pre-determined intervals using a simple human-friendly syntax.

Features
--------

- Simple fluent/builder API
- Allows single or multi thread updates
- Thread-safe
- Single-file header only
- No dependencies 

Requirements
------------

**cpptimer** requires a C++11 compiler only with `<ctime>` and `<chrono>` libraries.

Usage
-----

Creating the timer manager.

```c++

    timer::manager mgr;
```

Registering tasks

```cpp

	mgr.interval([]() {
		cout << "Happens every 5 second" << endl;
	}, std::chrono::seconds(5));

	mgr.timeout([]() {
		cout << "Happens once after 10 seconds" << endl;
	}, std::chrono::seconds(10));

	mgr.repeat([]() {
		cout << "Happens 5 times every 3 seconds" << endl;
	}, std::chrono::seconds(3), 5);

```

Canceling a task

```cpp
    
	auto id = mgr.timeout(func, std::chrono::seconds(60));
	...
	mgr.cancel(id);

```

Updating the timers on your own thread

```cpp

    while (true)
	{
		mgr.update();
		boost::this_thread::sleep_for(
			boost::chrono::milliseconds(200));
	}

```

Leaving updates to the timer 

*Note that this will create an additional thread and all updates will run under manager's thread*


```cpp

	// default interval (250ms)
	mgr.start();

	// or with a custom interval for each update
	mgr.start(std::chrono::milliseconds(200)); 
	
```

Distributed under the MIT license. See ``LICENSE`` for more information.
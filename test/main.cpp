#include <stdio.h>

#include <threadutil/event_loop.h>
#include <threadutil/async.h>
#include <threadutil/event_receiver.h>
#include <threadutil/shared_singleton.h>

void sum(EventLoop *e, int x, int y, std::function<void(char *err, int res)> callback)
{
	int r = x + y;
	e->immediate([=]() -> void {
		callback(NULL, r);
	});
}

struct tester : EventReceiver
{
public:
	tester(EventLoop *loop) : EventReceiver(loop) { }
	~tester() { printf("Tester destructor.\n"); }

	void test()
	{
		EventReceiverFunction f1(this, []()-> void {
			printf("Should not see this, since tester t is destroyed already.\n");
		});
		eventLoop()->timeout([f1]()-> void {
			printf("Testing dead event receiver function now.\n");
			f1.immediate();
		}, std::chrono::milliseconds(2000));

		EventReceiverFunction f2(this, []()-> void {
			printf("Live event okay.\n");
		});
		eventLoop()->immediate([f2]()-> void {
			printf("Testing live event receiver function now.\n");
			f2.immediate();
		});

		onTestCallback(50, 900);
	}

	EventCallback<tester, int, int> onTestCallback;

};

class neato : public tester, public SharedSingleton<neato>
{
public:
	neato(EventLoop *loop) : tester(loop) { printf("Neato constructor.\n"); }
	~neato() { printf("Neato destructor.\n"); }
};

int main()
{
	EventLoop e;

	e.immediate([&e]() -> void {
		sum(&e, 50, 60, [](char *err, int res) -> void {
			printf("sum: %i\n", res);
		});
	});
	Async::parallel([&e](std::function<void()> callback) -> void {
		// callback();
		Async::parallel([&e](std::function<void()> callback) -> void {
			/*std::thread thread([&e, callback]() -> void {
				printf("a1\n");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				printf("a2\n");
				e.setImmediate(callback);
			});
			thread.detach();*/
			/*e.thread([]() -> void {
				printf("a1\n");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				printf("a2\n");
			}, callback);*/
			printf("a1\n");
			e.timeout([callback]() -> void {
				printf("a2\n");
				callback();
			}, std::chrono::milliseconds(1000));
		}, [](std::function<void()> callback) -> void {
			printf("b\n");
			callback();
		}, callback);
	}, [](std::function<void()> callback) -> void {
		printf("c\n");
		callback();
	}, []() -> void {
		printf("completion\n");
		return;
	});

	e.interval([&e]() -> void {
		printf("once\n");
		e.cancel();
	}, std::chrono::milliseconds(500));

	e.timeout([]() -> void {
		printf("5\n");
	}, std::chrono::milliseconds(5000));
	e.timeout([]() -> void {
		printf("1\n");
	}, std::chrono::milliseconds(1000));
	/* int three = */ e.timeout([]() -> void {
		printf("3\n");
	}, std::chrono::milliseconds(3000));
	e.timeout([]() -> void {
		printf("2\n");
	}, std::chrono::milliseconds(2000));
	e.timeout([&e]() -> void {
		printf("4\n");
		e.stop();
	}, std::chrono::milliseconds(4000));

	// e.clear(three);

	AsyncParallel ap(e);
	ap.call([&e](std::function<void()> callback) -> void {
		std::thread thread([&e, callback]() -> void {
			printf("ccca1\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			printf("ccca2\n");
			e.immediate(callback);
		});
		thread.detach();
	});
	ap.call([&e](std::function<void()> callback) -> void {
		std::thread thread([&e, callback]() -> void {
			printf("cccb1\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			printf("cccb2\n");
			e.immediate(callback);
		});
		thread.detach();
	});
	ap.completed([]() -> void {
		printf("ccccccompletion\n");
		return;
	});

	printf("Create tester t\n");
	tester *tp = new tester(&e);

	; {
		neato::Instance inst = neato::instance(&e);
		neato::instance(&e);
		neato::instance(&e);
		tp->onTestCallback(inst, [](int a, int b) -> void {
			printf("This callback is dead already, should not show up %i %i\n", a, b);
		});
	}
	tp->onTestCallback(tp, [](int a, int b) -> void {
		printf("Received callback %i %i\n", a, b);
	});

	tp->test();
	e.immediate([&]() -> void {
		printf("Destroy tester t\n");
		EventReceiverHandle h1 = tp->eventReceiverHandle();
		if (!h1.alive()) printf("This h1 should be alive, there's an issue\n");
		delete tp; // Destroy previous tester
		tp = new tester(&e);
		EventReceiverHandle h2 = tp->eventReceiverHandle();
		if (h1.alive()) printf("This h1 should not be alive, there's an issue\n");
		if (!h2.alive()) printf("This h2 should be alive, there's an issue\n");
	});

	e.runSync();

	return 0;
}

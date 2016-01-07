#include <stdio.h>

#include <threadutil/eventloop.h>
#include <threadutil/async.h>

void sum(EventLoop *e, int x, int y, std::function<void(char *err, int res)> callback)
{
	int r = x + y;
	e->immediate([=]() -> void {
		callback(NULL, r);
	});
}

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

	e.timeout([]() -> void {
		printf("5\n");
	}, std::chrono::milliseconds(5000));
	e.timeout([]() -> void {
		printf("1\n");
	}, std::chrono::milliseconds(1000));
	int three = e.timeout([]() -> void {
		printf("3\n");
	}, std::chrono::milliseconds(3000));
	e.timeout([]() -> void {
		printf("2\n");
	}, std::chrono::milliseconds(2000));
	e.timeout([]() -> void {
		printf("4\n");
	}, std::chrono::milliseconds(4000));

	e.clear(three);

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

	e.runSync();

	return 0;
}

#include <stdio.h>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <functional>

#include <queue>

class EventLoop
{
public:
	EventLoop() : m_Running(false)
	{
		
	}

	~EventLoop()
	{
		stop();
		// todo: wait
	}

	void run()
	{
		m_Running = true;
		std::thread t(&EventLoop::loop, this);
		t.detach();
	}

	void runSync()
	{
		m_Running = true;
		loop();
	}

	void stop() // thread-safe
	{
		m_Running = false;
	}

public:
	void setImmediate(std::function<void()> f) // thread-safe
	{
		std::unique_lock<std::mutex> lock(m_QueueLock);
		m_Immediate.push(f);
		poke();
	}

	template<class rep, class period>
	void setTimeout(std::function<void()> f, const std::chrono::duration<rep, period>& time) // thread-safe
	{
		timeout_func tf;
		tf.f = f;
		tf.time = std::chrono::steady_clock::now() + time;
		; {
			std::unique_lock<std::mutex> lock(m_QueueTimeoutLock);
			m_Timeout.push(tf);
			poke();
		}
	}

public:
	void thread(std::function<void()> f, std::function<void()> callback)
	{
		std::thread t([this, f, callback]() -> void {
			f();
			setImmediate(callback);
		});
		t.detach();
	}

private:
	void loop()
	{
		printf("Thread\n");

		while (m_Running)
		{
			printf("Loop\n");

			for (;;)
			{
				m_QueueLock.lock();
				if (!m_Immediate.size())
				{
					m_QueueLock.unlock();
					break;
				}
				std::function<void()> f = m_Immediate.front();
				m_Immediate.pop();
				m_QueueLock.unlock();
				f();
			}

			bool poked = false;
			for (;;)
			{
				m_QueueTimeoutLock.lock();
				if (!m_Timeout.size())
				{
					m_QueueTimeoutLock.unlock();
					break;
				}
				timeout_func tf = m_Timeout.top();
				if (tf.time > std::chrono::steady_clock::now())
				{
					m_QueueTimeoutLock.unlock();
					; {
						std::unique_lock<std::mutex> lock(m_PokeLock);
						m_PokeCond.wait_until(lock, tf.time);
					}
					poked = true;
					break;
				}
				m_Timeout.pop();
				m_QueueTimeoutLock.unlock();
				tf.f();
			}

			if (!poked)
			{
				std::unique_lock<std::mutex> lock(m_PokeLock);
				m_PokeCond.wait(lock);
			}
		}
	}

	void poke() // private
	{
		m_PokeCond.notify_one();
	}

private:
	struct parallel_state
	{
		std::function<void()> callback;
		int remaining;
	};

	template<typename Tf>
	inline void parallelsub(std::shared_ptr<parallel_state> state, Tf f)
	{
		if (state->remaining)
			state->callback = f;
		else if (!state->callback)
			f();
	}

	template<typename Tf, typename... Tfv>
	inline void parallelsub(std::shared_ptr<parallel_state> state, Tf f, Tfv... fv)
	{
		++state->remaining;
		f([state]() -> void {
			--state->remaining;
			if (!state->remaining && state->callback)
				state->callback();
		});
		parallelsub(state, fv...);
	}

public:
	template<typename... Tfv>
	void parallel(Tfv... fv) // only call from event loop itself
	{
		std::shared_ptr<parallel_state> state = std::shared_ptr<parallel_state>(new parallel_state());
		state->remaining = 0;
		parallelsub(state, fv...);
	}

private:
	struct timeout_func
	{
		std::function<void()> f;
		std::chrono::steady_clock::time_point time;

		bool operator <(const timeout_func &o) const
		{
			return time > o.time;
		}

	};

private:
	volatile bool m_Running;
	std::mutex m_PokeLock;
	std::condition_variable m_PokeCond;

	std::mutex m_QueueLock;
	std::queue<std::function<void()>> m_Immediate;
	std::mutex m_QueueTimeoutLock;
	std::priority_queue<timeout_func> m_Timeout;

};

void sum(EventLoop *e, int x, int y, std::function<void(char *err, int res)> callback)
{
	int r = x + y;
	e->setImmediate([=]() -> void {
		callback(NULL, r);
	});
}

int main()
{
	EventLoop e;

	e.setImmediate([&e]() -> void {
		sum(&e, 50, 60, [](char *err, int res) -> void {
			printf("sum: %i\n", res);
		});
	});
	e.parallel([&e](std::function<void()> callback) -> void {
		// callback();
		e.parallel([&e](std::function<void()> callback) -> void {
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
			e.setTimeout([callback]() -> void {
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

	e.setTimeout([]() -> void {
		printf("5\n");
	}, std::chrono::milliseconds(5000));
	e.setTimeout([]() -> void {
		printf("1\n");
	}, std::chrono::milliseconds(1000));
	e.setTimeout([]() -> void {
		printf("3\n");
	}, std::chrono::milliseconds(3000));
	e.setTimeout([]() -> void {
		printf("2\n");
	}, std::chrono::milliseconds(2000));
	e.setTimeout([]() -> void {
		printf("4\n");
	}, std::chrono::milliseconds(4000));

	e.runSync();

	return 0;
}

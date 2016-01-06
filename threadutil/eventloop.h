
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
		while (m_Running)
		{
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

/* end of file */

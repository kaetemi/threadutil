
#include <functional>
#include <atomic>

class Async
{
private:
	struct parallel_state
	{
		std::function<void()> callback;
		int remaining;
	};

	template<typename Tf>
	inline static void parallelsub(std::shared_ptr<parallel_state> state, Tf f)
	{
		if (state->remaining)
			state->callback = f;
		else if (!state->callback)
			f();
	}

	template<typename Tf, typename... Tfv>
	inline static void parallelsub(std::shared_ptr<parallel_state> state, Tf f, Tfv... fv)
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
	void static parallel(Tfv... fv) // only call from event loop itself
	{
		std::shared_ptr<parallel_state> state = std::shared_ptr<parallel_state>(new parallel_state());
		state->remaining = 0;
		parallelsub(state, fv...);
	}

};

// Not thread safe (yet)
class AsyncParallel
{
public:
	AsyncParallel(EventLoop &e) : m_EventLoop(e), m_Remaining(0)
	{

	}

	template<typename TFunc>
	inline void call(TFunc f)
	{
		++m_Remaining;
		f([this]() -> void {
			--m_Remaining;
			if (!m_Remaining && m_Completed)
			{
				m_Completed();
				m_Completed = std::function<void()>();
			}
		});
	}

	template<typename TFunc>
	inline void defer(TFunc f)
	{
		++m_Remaining;
		m_EventLoop.immediate([this, f]() -> void {
			f([this]() -> void {
				--m_Remaining;
				if (!m_Remaining && m_Completed)
				{
					m_Completed();
					m_Completed = std::function<void()>();
				}
			});
		});
	}

	template<typename TFunc>
	inline void completed(TFunc f)
	{
		if (m_Remaining)
			m_Completed = f;
		else if (!m_Completed)
			f();
	}

private:
	std::function<void()> m_Completed;
	int m_Remaining;
	EventLoop &m_EventLoop;

};

/* end of file */


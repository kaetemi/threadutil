/*

Copyright (C) 2016-2017  by authors
Author: Jan Boon <jan.boon@kaetemi.be>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef THREADUTIL_EVENT_RECEIVER_H
#define THREADUTIL_EVENT_RECEIVER_H

#include "event_loop.h"

struct EventReceiverHandle
{
public:
	inline EventReceiverHandle() { }
	inline bool alive() const { return m_Data && m_Data->Alive; }
	inline operator bool() const { return alive(); }
	inline EventLoop *eventLoop() const { return m_Data->EventLoop; }
	inline bool immediate(const std::function<void()> &f) const { if (alive()) { eventLoop()->immediate(f); return true; } return false; }

private:
	friend class EventReceiver;

	struct Data
	{
	public:
		Data(EventLoop *loop) : Alive(true), EventLoop(loop) { }
		bool Alive;
		EventLoop *EventLoop;
	};

	inline EventReceiverHandle(EventLoop *loop) : m_Data(new Data(loop)) { }
	inline void p_destroyed() { m_Data->Alive = false; }
	inline void p_invalidate() { m_Data.reset(); }
	inline void p_init(EventLoop *loop) { m_Data = std::shared_ptr<Data>(new Data(loop)); }

	std::shared_ptr<Data> m_Data;

};

//! The purpose of event receiver is to have a way to callback while keeping in mind the lifetime of the response object
class EventReceiver
{
public:
	EventReceiver(EventLoop *loop) : m_Handle(loop)
	{

	}

	~EventReceiver()
	{
		m_Handle.p_destroyed(); 
	}

	inline const EventReceiverHandle &eventReceiverHandle() const { return m_Handle; }
	inline EventLoop *eventLoop() const { return m_Handle.m_Data->EventLoop; }

	EventReceiver(const EventReceiver &other)
	{
		m_Handle.p_init(other.eventLoop());
	}

	EventReceiver &operator=(const EventReceiver &other)
	{
		if (this != &other)
			m_Handle.p_init(other.eventLoop());
		return *this;
	}

	EventReceiver(EventReceiver &&other)
	{
		m_Handle = other.m_Handle;
		other.m_Handle.p_invalidate();
	}

	EventReceiver &operator=(EventReceiver &&other)
	{
		if (this != &other)
		{
			m_Handle = other.m_Handle;
			other.m_Handle.p_invalidate();
		}
		return *this;
	}

private:
	EventReceiverHandle m_Handle;

};

struct EventReceiverFunction
{
public:
	inline EventReceiverFunction() { }
	inline EventReceiverFunction(const EventReceiver *receiver, const EventFunction &f) : m_Handle(receiver->eventReceiverHandle()), m_Function(f) { }
	inline void immediate() const { m_Handle.immediate(m_Function); }

private:
	EventReceiverHandle m_Handle;
	EventFunction m_Function;

};

#include "atomic_lock.h"

template<typename ... TParams>
struct EventCallbackFunction
{
public:
	typedef std::function<void(TParams ...)> FunctionType;

	inline EventCallbackFunction() { }
	inline EventCallbackFunction(const EventReceiver *receiver, const FunctionType &f) : m_Handle(receiver->eventReceiverHandle()), m_Function(f) { }
	inline bool immediate(TParams ... args) const { return m_Handle.immediate([=]() -> void { m_Function(args ...); }); }

private:
	EventReceiverHandle m_Handle;
	FunctionType m_Function;

};

template<class TFriend, typename ... TParams>
struct EventCallback
{
private:
	friend TFriend;
	typedef EventCallbackFunction<TParams ...> CallbackType;

public:
	typedef typename CallbackType::FunctionType FunctionType;
	inline EventCallback() { }

	void operator() (EventReceiver *receiver, FunctionType f)
	{
		m_Lock.lock();
		m_Callbacks.push_back(CallbackType(receiver, f));
		m_Lock.unlock();
	}

private:
	void operator() (TParams ... args)
	{
		m_Lock.lock();
		for (int i = 0; i < m_Callbacks.size(); ++i)
		{
			const CallbackType &cb = m_Callbacks[i];
			if (!cb.immediate(args ...))
			{
				m_Callbacks.erase(m_Callbacks.begin() + i);
				--i;
			}
		}
		m_Lock.unlock();
	}

	AtomicLock m_Lock;
	std::vector<CallbackType> m_Callbacks;

};

#endif /* THREADUTIL_EVENT_RECEIVER_H */

/* end of file */

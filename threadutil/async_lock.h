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

#ifndef THREADUTIL_ASYNC_LOCK_H
#define THREADUTIL_ASYNC_LOCK_H

#include <atomic>
#include <concurrent_queue.h>

#include "event_receiver.h"

//! Asynchronous locking mechanism which puts the provided function into the event loop as soon as the lock is available
class AsyncLock
{
public:
	inline void lock(const EventReceiver *receiver, EventFunction f)
	{
		m_Queue.push(EventReceiverFunction(receiver, f));
		progress();
	}

	inline void unlock()
	{
		m_Atomic.clear();
		progress();
	}

private:
	inline void progress()
	{
		if (!m_Atomic.test_and_set())
		{
			EventReceiverFunction f;
			if (m_Queue.try_pop(f))
				f.immediate();
			else
				m_Atomic.clear();
		}
	}

	std::atomic_flag m_Atomic;
	concurrency::concurrent_queue<EventReceiverFunction> m_Queue;

	AsyncLock &operator=(const AsyncLock&) = delete;
	AsyncLock(const AsyncLock&) = delete;

};

#endif /* THREADUTIL_ASYNC_LOCK_H */

/* end of file */

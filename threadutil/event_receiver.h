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
	inline void immediate(const std::function<void()> &f) { if (alive()) eventLoop()->immediate(f); }

private:
	friend class EventReceiver;

	struct Data
	{
		bool Alive;
		EventLoop *EventLoop;
	};

	inline EventReceiverHandle(EventLoop *loop) : m_Data((Data){true, loop}) { }
	inline void p_destroyed() { m_Data->Alive = false; }

	std::shared_ptr<Data> m_Data;

};

struct EventReceiverFunction
{
public:
	inline EventReceiverFunction() { }
	inline EventReceiverFunction(const EventReceiver *receiver, const EventFunction &f) : m_Handle(receiver->eventReceiverHandle()), m_Function(f) { }
	inline void immediate() { m_Handle.immediate(m_Function); }

private:
	EventReceiverHandle m_Handle;
	EventFunction m_Function;

};

class EventReceiver
{
public:
	EventReceiver(EventLoop *loop) : m_Handle(loop) { }
	~EventReceiver() { m_Handle.p_destroyed(); }
	inline const EventReceiverHandle &eventReceiverHandle() const { return m_Handle; }
	inline EventLoop *eventLoop() const { return m_Handle.m_Data->EventLoop; }

private:
	EventReceiverHandle m_Handle;

};

#endif /* THREADUTIL_EVENT_RECEIVER_H */

/* end of file */

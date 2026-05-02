/*
	Sherer 2017-2024 (C) - Revisioned by martysama0134 for c++20 support.
	Feel free to use it on your own.
	But ffs don`t remove this.
*/

#include "stdafx.h"
#include "config.h"
#include "EventFunctionHandler.h"

uint32_t CEventFunctionHandler::AllocateId()
{
	if (!m_freeIds.empty())
	{
		const uint32_t id = m_freeIds.back();
		m_freeIds.pop_back();
		auto& record = m_events[id];
		record.func = nullptr;
		record.name.clear();
		return id;
	}
	const uint32_t id = static_cast<uint32_t>(m_events.size());
	m_events.emplace_back();
	return id;
}

void CEventFunctionHandler::ReleaseId(const uint32_t id)
{
	auto& record = m_events[id];
	record.active = false;
	++record.generation;
	m_freeIds.push_back(id);
}

const CEventFunctionHandler::SEventRecord* CEventFunctionHandler::GetRecordByName(const std::string_view name) const
{
	if (const auto it = m_nameToId.find(name); it != m_nameToId.end())
	{
		const auto& record = m_events[it->second];
		if (record.active)
			return &record;
	}
	return nullptr;
}

CEventFunctionHandler::SEventRecord* CEventFunctionHandler::GetRecordByName(const std::string_view name)
{
	return const_cast<SEventRecord*>(std::as_const(*this).GetRecordByName(name));
}

CEventFunctionHandler::EventHandle CEventFunctionHandler::ResolveHandle(const std::string_view name) const
{
	if (const auto it = m_nameToId.find(name); it != m_nameToId.end())
	{
		const auto& record = m_events[it->second];
		if (record.active)
			return EventHandle{it->second, record.generation};
	}
	return {};
}

void CEventFunctionHandler::RebuildQueueIfNeeded(MinHeap& queue, const ETimeBase timeBase, const size_t staleCount, const size_t preDrainSize)
{
	if (preDrainSize == 0 || staleCount * 2 <= preDrainSize)
		return;

	MinHeap fresh;
	for (uint32_t id = 0; id < static_cast<uint32_t>(m_events.size()); ++id)
	{
		const auto& record = m_events[id];
		if (record.active && record.timeBase == timeBase)
			fresh.push(SHeapEntry{record.dueAt, id, record.heapSeq});
	}
	queue = std::move(fresh);
}

void CEventFunctionHandler::Destroy()
{
	m_freeIds.clear();
	for (uint32_t id = 0; id < static_cast<uint32_t>(m_events.size()); ++id)
	{
		auto& record = m_events[id];
		record.active = false;
		++record.generation;
		m_freeIds.push_back(id);
	}
	m_nameToId.clear();
	m_secondsQueue = MinHeap{};
	m_pulseQueue = MinHeap{};
}

CEventFunctionHandler::EventHandle CEventFunctionHandler::AddEvent(EventCallback func, const std::string_view name, const size_t time)
{
	if (name.empty() || !func || m_nameToId.contains(name))
		return {};

	const auto now = get_global_time();
	if (time > 0 && now + time < now)
		return {};

	const uint32_t id = AllocateId();
	auto& record = m_events[id];
	record.func = std::move(func);
	record.timeBase = ETimeBase::Seconds;
	record.dueAt = now + time;
	record.loopTime = 0;
	record.name = std::string(name);
	record.active = true;
	++record.heapSeq;

	m_nameToId.emplace(record.name, id);
	m_secondsQueue.push(SHeapEntry{record.dueAt, id, record.heapSeq});
	return EventHandle{id, record.generation};
}

CEventFunctionHandler::EventHandle CEventFunctionHandler::AddPulseEvent(EventCallback func, const std::string_view name, int32_t pulseDelay)
{
	if (name.empty() || !func || m_nameToId.contains(name))
		return {};

	if (pulseDelay <= 0)
		pulseDelay = 1;

	const auto now = thecore_pulse();
	const auto dueAt = now + static_cast<size_t>(pulseDelay);
	if (dueAt < now)
		return {};

	const uint32_t id = AllocateId();
	auto& record = m_events[id];
	record.func = std::move(func);
	record.timeBase = ETimeBase::Pulses;
	record.dueAt = dueAt;
	record.loopTime = 0;
	record.name = std::string(name);
	record.active = true;
	++record.heapSeq;

	m_nameToId.emplace(record.name, id);
	m_pulseQueue.push(SHeapEntry{record.dueAt, id, record.heapSeq});
	return EventHandle{id, record.generation};
}

CEventFunctionHandler::EventHandle CEventFunctionHandler::AddLoopEvent(EventCallback func, const std::string_view name, const size_t interval)
{
	if (name.empty() || !func || interval == 0 || m_nameToId.contains(name))
		return {};

	const auto now = get_global_time();
	if (now + interval < now)
		return {};

	const uint32_t id = AllocateId();
	auto& record = m_events[id];
	record.func = std::move(func);
	record.timeBase = ETimeBase::Seconds;
	record.dueAt = now + interval;
	record.loopTime = static_cast<int64_t>(interval);
	record.name = std::string(name);
	record.active = true;
	++record.heapSeq;

	m_nameToId.emplace(record.name, id);
	m_secondsQueue.push(SHeapEntry{record.dueAt, id, record.heapSeq});
	return EventHandle{id, record.generation};
}

bool CEventFunctionHandler::RemoveEvent(const EventHandle handle)
{
	if (!handle || handle.id >= static_cast<uint32_t>(m_events.size()))
		return false;

	auto& record = m_events[handle.id];
	if (!record.active || record.generation != handle.generation)
		return false;

	const auto nameIt = m_nameToId.find(record.name);
	if (nameIt != m_nameToId.end())
		m_nameToId.erase(nameIt);
	ReleaseId(handle.id);
	return true;
}

bool CEventFunctionHandler::RemoveEvent(const std::string_view name)
{
	return RemoveEvent(ResolveHandle(name));
}

bool CEventFunctionHandler::DelayEvent(const EventHandle handle, const size_t newtime)
{
	if (!handle || handle.id >= static_cast<uint32_t>(m_events.size()))
		return false;

	auto& record = m_events[handle.id];
	if (!record.active || record.generation != handle.generation)
		return false;

	if (record.timeBase != ETimeBase::Seconds || record.loopTime != 0)
		return false;

	const auto now = get_global_time();
	if (newtime > 0 && now + newtime < now)
		return false;

	record.dueAt = now + newtime;
	++record.heapSeq;
	m_secondsQueue.push(SHeapEntry{record.dueAt, handle.id, record.heapSeq});
	return true;
}

bool CEventFunctionHandler::DelayEvent(const std::string_view name, const size_t newtime)
{
	return DelayEvent(ResolveHandle(name), newtime);
}

bool CEventFunctionHandler::FindEvent(const EventHandle handle) const
{
	if (!handle || handle.id >= static_cast<uint32_t>(m_events.size()))
		return false;

	const auto& record = m_events[handle.id];
	return record.active && record.generation == handle.generation;
}

bool CEventFunctionHandler::FindEvent(const std::string_view name) const
{
	return GetRecordByName(name) != nullptr;
}

size_t CEventFunctionHandler::GetDelay(const EventHandle handle) const
{
	if (!handle || handle.id >= static_cast<uint32_t>(m_events.size()))
		return 0;

	const auto& record = m_events[handle.id];
	if (!record.active || record.generation != handle.generation || record.timeBase != ETimeBase::Seconds)
		return 0;

	const auto now = get_global_time();
	return record.dueAt > now ? record.dueAt - now : 0;
}

size_t CEventFunctionHandler::GetDelay(const std::string_view name) const
{
	return GetDelay(ResolveHandle(name));
}

size_t CEventFunctionHandler::GetPulseDelay(const EventHandle handle) const
{
	if (!handle || handle.id >= static_cast<uint32_t>(m_events.size()))
		return 0;

	const auto& record = m_events[handle.id];
	if (!record.active || record.generation != handle.generation || record.timeBase != ETimeBase::Pulses)
		return 0;

	const auto now = thecore_pulse();
	return record.dueAt > now ? record.dueAt - now : 0;
}

size_t CEventFunctionHandler::GetPulseDelay(const std::string_view name) const
{
	return GetPulseDelay(ResolveHandle(name));
}

bool CEventFunctionHandler::IsPulseEvent(const EventHandle handle) const
{
	if (!handle || handle.id >= static_cast<uint32_t>(m_events.size()))
		return false;

	const auto& record = m_events[handle.id];
	return record.active && record.generation == handle.generation && record.timeBase == ETimeBase::Pulses;
}

bool CEventFunctionHandler::IsPulseEvent(const std::string_view name) const
{
	return IsPulseEvent(ResolveHandle(name));
}

void CEventFunctionHandler::Process()
{
	if (m_processing)
		return;

	m_processing = true;
	struct ScopeGuard
	{
		bool& flag;
		~ScopeGuard() { flag = false; }
	} guard{m_processing};

	const auto currentSeconds = get_global_time();
	const auto currentPulse = thecore_pulse();

	struct SCollectedEvent
	{
		uint32_t eventId;
		uint32_t generation;
		ETimeBase timeBase;
		EventCallback func;
	};

	std::vector<SCollectedEvent> collected;

	auto drainQueue = [&](MinHeap& queue, const ETimeBase timeBase, const size_t now)
	{
		const size_t preDrainSize = queue.size();
		size_t staleCount = 0;
		while (!queue.empty() && queue.top().dueAt <= now)
		{
			const SHeapEntry entry = queue.top();
			queue.pop();

			if (entry.eventId >= static_cast<uint32_t>(m_events.size()))
			{
				++staleCount;
				continue;
			}

			auto& record = m_events[entry.eventId];
			if (!record.active || record.heapSeq != entry.heapSeq || record.timeBase != timeBase)
			{
				++staleCount;
				continue;
			}

			collected.push_back(SCollectedEvent{entry.eventId, record.generation, timeBase, record.func});
		}
		RebuildQueueIfNeeded(queue, timeBase, staleCount, preDrainSize);
	};

	drainQueue(m_secondsQueue, ETimeBase::Seconds, currentSeconds);
	drainQueue(m_pulseQueue, ETimeBase::Pulses, currentPulse);

	for (const auto& [eventId, generation, timeBase, func] : collected)
	{
		if (eventId >= static_cast<uint32_t>(m_events.size()))
			continue;

		const int32_t result = func();

		if (eventId >= static_cast<uint32_t>(m_events.size()))
			continue;

		auto& postRecord = m_events[eventId];
		if (!postRecord.active || postRecord.generation != generation)
			continue;

		const size_t now = (timeBase == ETimeBase::Pulses) ? currentPulse : currentSeconds;
		MinHeap& queue = (timeBase == ETimeBase::Pulses) ? m_pulseQueue : m_secondsQueue;

		if (result > 0)
		{
			const auto newDueAt = now + static_cast<size_t>(result);
			if (newDueAt < now)
			{
				const auto nameIt = m_nameToId.find(postRecord.name);
				if (nameIt != m_nameToId.end())
					m_nameToId.erase(nameIt);
				ReleaseId(eventId);
				continue;
			}
			postRecord.dueAt = newDueAt;
			++postRecord.heapSeq;
			queue.push(SHeapEntry{postRecord.dueAt, eventId, postRecord.heapSeq});
		}
		else if (postRecord.loopTime > 0)
		{
			postRecord.dueAt = now + static_cast<size_t>(postRecord.loopTime);
			++postRecord.heapSeq;
			queue.push(SHeapEntry{postRecord.dueAt, eventId, postRecord.heapSeq});
		}
		else
		{
			const auto nameIt = m_nameToId.find(postRecord.name);
			if (nameIt != m_nameToId.end())
				m_nameToId.erase(nameIt);
			ReleaseId(eventId);
		}
	}
}

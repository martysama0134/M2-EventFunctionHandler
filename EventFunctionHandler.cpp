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
	record.func = nullptr;
	record.name.clear();
	++record.generation;
	m_freeIds.push_back(id);
}

const CEventFunctionHandler::SEventRecord* CEventFunctionHandler::GetRecordByName(const std::string_view event_name) const
{
	if (const auto it = m_nameToId.find(event_name); it != m_nameToId.end())
	{
		const auto& record = m_events[it->second];
		if (record.active)
			return &record;
	}
	return nullptr;
}

CEventFunctionHandler::SEventRecord* CEventFunctionHandler::GetRecordByName(const std::string_view event_name)
{
	return const_cast<SEventRecord*>(std::as_const(*this).GetRecordByName(event_name));
}

void CEventFunctionHandler::RebuildQueueIfNeeded(MinHeap& queue, const ETimeBase timeBase, const size_t staleCount)
{
	if (queue.empty() || staleCount * 2 <= queue.size())
		return;

	MinHeap fresh;
	for (uint32_t id = 0; id < static_cast<uint32_t>(m_events.size()); ++id)
	{
		const auto& record = m_events[id];
		if (record.active && record.timeBase == timeBase)
			fresh.push(SHeapEntry{record.dueAt, id, record.generation});
	}
	queue = std::move(fresh);
}

void CEventFunctionHandler::Destroy()
{
	m_nameToId.clear();
	m_events.clear();
	m_freeIds.clear();
	m_secondsQueue = MinHeap{};
	m_pulseQueue = MinHeap{};
}

bool CEventFunctionHandler::AddEvent(std::function<void(SArgumentSupportImpl*)> func, const std::string_view event_name, const size_t time, const bool loop)
{
	if (m_nameToId.contains(event_name))
		return false;

	EventCallback wrappedFunc = [wrapped = std::move(func)](SArgumentSupportImpl* arg) -> long
	{
		wrapped(arg);
		return 0;
	};

	const uint32_t id = AllocateId();
	auto& record = m_events[id];
	record.func = std::move(wrappedFunc);
	record.timeBase = ETimeBase::Seconds;
	record.dueAt = get_global_time() + time;
	record.loopTime = loop ? static_cast<int64_t>(time) : 0;
	record.name = std::string(event_name);
	record.active = true;

	m_nameToId.emplace(record.name, id);
	m_secondsQueue.push(SHeapEntry{record.dueAt, id, record.generation});
	return true;
}

bool CEventFunctionHandler::AddPulseEvent(EventCallback func, const std::string_view event_name, long pulseDelay)
{
	if (m_nameToId.contains(event_name))
		return false;

	if (pulseDelay <= 0)
		pulseDelay = 1;

	const uint32_t id = AllocateId();
	auto& record = m_events[id];
	record.func = std::move(func);
	record.timeBase = ETimeBase::Pulses;
	record.dueAt = thecore_pulse() + static_cast<size_t>(pulseDelay);
	record.loopTime = 0;
	record.name = std::string(event_name);
	record.active = true;

	m_nameToId.emplace(record.name, id);
	m_pulseQueue.push(SHeapEntry{record.dueAt, id, record.generation});
	return true;
}

void CEventFunctionHandler::RemoveEvent(const std::string_view event_name)
{
	if (const auto it = m_nameToId.find(event_name); it != m_nameToId.end())
	{
		ReleaseId(it->second);
		m_nameToId.erase(it);
	}
}

void CEventFunctionHandler::DelayEvent(const std::string_view event_name, const size_t newtime)
{
	auto* record = GetRecordByName(event_name);
	if (!record || record->timeBase != ETimeBase::Seconds || record->loopTime != 0)
		return;

	const auto it = m_nameToId.find(event_name);
	const uint32_t id = it->second;

	++record->generation;
	record->dueAt = get_global_time() + newtime;
	m_secondsQueue.push(SHeapEntry{record->dueAt, id, record->generation});
}

bool CEventFunctionHandler::FindEvent(const std::string_view event_name) const
{
	return GetRecordByName(event_name) != nullptr;
}

DWORD CEventFunctionHandler::GetDelay(const std::string_view event_name) const
{
	const auto* record = GetRecordByName(event_name);
	if (!record || record->timeBase != ETimeBase::Seconds)
		return 0;

	const auto current_time = get_global_time();
	return record->dueAt > current_time ? static_cast<DWORD>(record->dueAt - current_time) : 0;
}

DWORD CEventFunctionHandler::GetPulseDelay(const std::string_view event_name) const
{
	const auto* record = GetRecordByName(event_name);
	if (!record || record->timeBase != ETimeBase::Pulses)
		return 0;

	const auto current_pulse = thecore_pulse();
	return record->dueAt > current_pulse ? static_cast<DWORD>(record->dueAt - current_pulse) : 0;
}

bool CEventFunctionHandler::IsPulseEvent(const std::string_view event_name) const
{
	const auto* record = GetRecordByName(event_name);
	return record && record->timeBase == ETimeBase::Pulses;
}

void CEventFunctionHandler::Process()
{
	if (m_nameToId.empty())
		return;

	const auto currentSeconds = get_global_time();
	const auto currentPulse = thecore_pulse();

	struct SCollectedEvent
	{
		EventCallback func;
		uint32_t eventId;
		uint32_t generation;
		ETimeBase timeBase;
	};

	std::vector<SCollectedEvent> collected;

	auto drainQueue = [&](MinHeap& queue, const ETimeBase timeBase, const size_t now)
	{
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
			if (!record.active || record.generation != entry.generation || record.timeBase != timeBase)
			{
				++staleCount;
				continue;
			}

			collected.push_back(SCollectedEvent{record.func, entry.eventId, entry.generation, timeBase});

			if (timeBase == ETimeBase::Seconds)
			{
				if (record.loopTime != 0)
				{
					++record.generation;
					record.dueAt = now + static_cast<size_t>(record.loopTime);
					queue.push(SHeapEntry{record.dueAt, entry.eventId, record.generation});
				}
				else
				{
					const auto nameIt = m_nameToId.find(record.name);
					if (nameIt != m_nameToId.end())
						m_nameToId.erase(nameIt);
					ReleaseId(entry.eventId);
				}
			}
		}
		RebuildQueueIfNeeded(queue, timeBase, staleCount);
	};

	drainQueue(m_secondsQueue, ETimeBase::Seconds, currentSeconds);
	drainQueue(m_pulseQueue, ETimeBase::Pulses, currentPulse);

	for (auto& [func, eventId, generation, timeBase] : collected)
	{
		const long result = func(nullptr);

		if (timeBase == ETimeBase::Pulses)
		{
			if (eventId >= static_cast<uint32_t>(m_events.size()))
				continue;

			auto& record = m_events[eventId];
			if (!record.active || record.generation != generation)
				continue;

			if (result > 0)
			{
				++record.generation;
				record.dueAt = currentPulse + static_cast<size_t>(result);
				m_pulseQueue.push(SHeapEntry{record.dueAt, eventId, record.generation});
			}
			else
			{
				const auto nameIt = m_nameToId.find(record.name);
				if (nameIt != m_nameToId.end())
					m_nameToId.erase(nameIt);
				ReleaseId(eventId);
			}
		}
	}
}

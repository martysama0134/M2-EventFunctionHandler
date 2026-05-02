/*
	Sherer 2017-2024 (C) - Revisioned by martysama0134 for c++20 support.
	Feel free to use it on your own.
	But ffs don`t remove this.
*/

#pragma once
#include "utils.h"
#include <cstdint>
#include <functional>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <type_traits>
#include <utility>
#include <vector>

class CEventFunctionHandler : public singleton<CEventFunctionHandler>
{
public:
	using EventCallback = std::function<int32_t()>;

	struct EventHandle
	{
		uint32_t id{UINT32_MAX};
		uint32_t generation{};
		explicit operator bool() const noexcept { return id != UINT32_MAX; }
		bool operator==(const EventHandle&) const noexcept = default;
	};

	CEventFunctionHandler() = default;
	virtual ~CEventFunctionHandler() = default;

	EventHandle AddEvent(EventCallback func, std::string_view name, size_t time);
	EventHandle AddPulseEvent(EventCallback func, std::string_view name, int32_t pulseDelay);
	EventHandle AddLoopEvent(EventCallback func, std::string_view name, size_t interval);

	bool RemoveEvent(EventHandle handle);
	bool DelayEvent(EventHandle handle, size_t newtime);
	bool FindEvent(EventHandle handle) const;
	size_t GetDelay(EventHandle handle) const;
	size_t GetPulseDelay(EventHandle handle) const;
	bool IsPulseEvent(EventHandle handle) const;

	bool RemoveEvent(std::string_view name);
	bool DelayEvent(std::string_view name, size_t newtime);
	bool FindEvent(std::string_view name) const;
	size_t GetDelay(std::string_view name) const;
	size_t GetPulseDelay(std::string_view name) const;
	bool IsPulseEvent(std::string_view name) const;

	void Destroy();
	void Process();

private:
	enum class ETimeBase : uint8_t { Seconds, Pulses };

	struct SEventRecord
	{
		bool active{false};
		ETimeBase timeBase{ETimeBase::Seconds};
		uint32_t generation{};
		uint32_t heapSeq{};
		size_t dueAt{};
		int64_t loopTime{};
		std::string name;
		EventCallback func;
	};

	struct SHeapEntry
	{
		size_t dueAt{};
		uint32_t eventId{};
		uint32_t heapSeq{};
		bool operator>(const SHeapEntry& rhs) const noexcept { return dueAt > rhs.dueAt; }
	};

	using MinHeap = std::priority_queue<SHeapEntry, std::vector<SHeapEntry>, std::greater<SHeapEntry>>;

	uint32_t AllocateId();
	void ReleaseId(uint32_t id);
	const SEventRecord* GetRecordByName(std::string_view name) const;
	SEventRecord* GetRecordByName(std::string_view name);
	EventHandle ResolveHandle(std::string_view name) const;
	void RebuildQueueIfNeeded(MinHeap& queue, ETimeBase timeBase, size_t staleCount, size_t preDrainSize);

	struct StringHash
	{
		using is_transparent = void;
		size_t operator()(std::string_view sv) const noexcept
		{
			return std::hash<std::string_view>{}(sv);
		}
	};

	std::unordered_map<std::string, uint32_t, StringHash, std::equal_to<>> m_nameToId;
	std::vector<SEventRecord> m_events;
	std::vector<uint32_t> m_freeIds;
	MinHeap m_secondsQueue;
	MinHeap m_pulseQueue;
	bool m_processing{false};
};

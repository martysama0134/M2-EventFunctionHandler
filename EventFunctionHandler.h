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
#include <vector>

struct SArgumentSupportImpl {}; // keep for v1 compatibility

struct StringHash
{
	using is_transparent = void;
	size_t operator()(std::string_view sv) const noexcept
	{
		return std::hash<std::string_view>{}(sv);
	}
};

class CEventFunctionHandler : public singleton<CEventFunctionHandler>
{
public:
	enum class ETimeBase : uint8_t
	{
		Seconds,
		Pulses,
	};

	using EventCallback = std::function<long(SArgumentSupportImpl*)>;

	struct SEventRecord
	{
		EventCallback func;
		ETimeBase timeBase{ETimeBase::Seconds};
		size_t dueAt{};
		int64_t loopTime{};
		uint32_t generation{};
		std::string name;
		bool active{false};
	};

	struct SHeapEntry
	{
		size_t dueAt{};
		uint32_t eventId{};
		uint32_t generation{};

		bool operator>(const SHeapEntry& rhs) const noexcept
		{
			return dueAt > rhs.dueAt;
		}
	};

	using MinHeap = std::priority_queue<SHeapEntry, std::vector<SHeapEntry>, std::greater<SHeapEntry>>;

	CEventFunctionHandler() = default;
	virtual ~CEventFunctionHandler() = default;

	void Destroy();
	bool AddEvent(std::function<void(SArgumentSupportImpl*)> func, std::string_view event_name, size_t time, bool loop = false);
	bool AddPulseEvent(EventCallback func, std::string_view event_name, long pulseDelay);
	void RemoveEvent(std::string_view event_name);
	void DelayEvent(std::string_view event_name, size_t newtime);
	bool FindEvent(std::string_view event_name) const;
	DWORD GetDelay(std::string_view event_name) const;
	DWORD GetPulseDelay(std::string_view event_name) const;
	bool IsPulseEvent(std::string_view event_name) const;
	void Process();

private:
	uint32_t AllocateId();
	void ReleaseId(uint32_t id);
	const SEventRecord* GetRecordByName(std::string_view event_name) const;
	SEventRecord* GetRecordByName(std::string_view event_name);
	void RebuildQueueIfNeeded(MinHeap& queue, ETimeBase timeBase, size_t staleCount);

	std::unordered_map<std::string, uint32_t, StringHash, std::equal_to<>> m_nameToId;
	std::vector<SEventRecord> m_events;
	std::vector<uint32_t> m_freeIds;
	MinHeap m_secondsQueue;
	MinHeap m_pulseQueue;
};

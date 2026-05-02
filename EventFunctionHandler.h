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

struct SArgumentSupportImpl {};

class CEventFunctionHandler : public singleton<CEventFunctionHandler>
{
public:
	using EventCallback = std::function<int32_t()>;

	struct EventHandle
	{
		explicit operator bool() const noexcept { return m_id != UINT32_MAX; }
		bool operator==(const EventHandle&) const noexcept = default;

	private:
		uint32_t m_id{UINT32_MAX};
		uint32_t m_generation{};
		friend class CEventFunctionHandler;
	};

	CEventFunctionHandler() = default;
	virtual ~CEventFunctionHandler() = default;

	template<typename F>
	EventHandle AddEvent(F&& func, std::string_view name, size_t time, bool loop = false)
	{
		auto cb = WrapCallback(std::forward<F>(func));
		if (!cb) return {};
		if (loop)
			return AddLoopEventImpl(std::move(cb), name, time);
		return AddEventImpl(std::move(cb), name, time);
	}

	template<typename F>
	EventHandle AddPulseEvent(F&& func, std::string_view name, int32_t pulseDelay)
	{
		auto cb = WrapCallback(std::forward<F>(func));
		if (!cb) return {};
		return AddPulseEventImpl(std::move(cb), name, pulseDelay);
	}

	template<typename F>
	EventHandle AddLoopEvent(F&& func, std::string_view name, size_t interval)
	{
		auto cb = WrapCallback(std::forward<F>(func));
		if (!cb) return {};
		return AddLoopEventImpl(std::move(cb), name, interval);
	}

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

	template<typename F>
	static EventCallback WrapCallback(F&& func)
	{
		using Fn = std::decay_t<F>;

		if constexpr (std::is_null_pointer_v<Fn>)
		{
			return nullptr;
		}
		else if constexpr (std::is_pointer_v<Fn> || std::is_member_pointer_v<Fn>)
		{
			if (!func) return nullptr;
			if constexpr (std::is_invocable_v<Fn&, SArgumentSupportImpl*>)
			{
				using R = std::invoke_result_t<Fn&, SArgumentSupportImpl*>;
				if constexpr (std::is_void_v<R>)
					return [f = std::forward<F>(func)]() mutable -> int32_t { f(nullptr); return 0; };
				else
					return [f = std::forward<F>(func)]() mutable -> int32_t { return static_cast<int32_t>(f(nullptr)); };
			}
			else if constexpr (std::is_invocable_v<Fn&>)
			{
				using R = std::invoke_result_t<Fn&>;
				if constexpr (std::is_void_v<R>)
					return [f = std::forward<F>(func)]() mutable -> int32_t { f(); return 0; };
				else
					return EventCallback(std::forward<F>(func));
			}
			else
			{
				static_assert(!sizeof(Fn), "Callback must be invocable with () or (SArgumentSupportImpl*)");
			}
		}
		else
		{
			if constexpr (requires { static_cast<bool>(func); })
			{
				if (!func) return nullptr;
			}

			if constexpr (std::is_invocable_v<Fn&, SArgumentSupportImpl*>)
			{
				using R = std::invoke_result_t<Fn&, SArgumentSupportImpl*>;
				if constexpr (std::is_void_v<R>)
					return [f = std::forward<F>(func)]() mutable -> int32_t { f(nullptr); return 0; };
				else
					return [f = std::forward<F>(func)]() mutable -> int32_t { return static_cast<int32_t>(f(nullptr)); };
			}
			else if constexpr (std::is_invocable_v<Fn&>)
			{
				using R = std::invoke_result_t<Fn&>;
				if constexpr (std::is_void_v<R>)
					return [f = std::forward<F>(func)]() mutable -> int32_t { f(); return 0; };
				else
					return EventCallback(std::forward<F>(func));
			}
			else
			{
				static_assert(!sizeof(Fn), "Callback must be invocable with () or (SArgumentSupportImpl*)");
			}
		}
	}

	EventHandle AddEventImpl(EventCallback func, std::string_view name, size_t time);
	EventHandle AddPulseEventImpl(EventCallback func, std::string_view name, int32_t pulseDelay);
	EventHandle AddLoopEventImpl(EventCallback func, std::string_view name, size_t interval);

	static EventHandle MakeHandle(uint32_t id, uint32_t generation) noexcept
	{
		EventHandle h;
		h.m_id = id;
		h.m_generation = generation;
		return h;
	}

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

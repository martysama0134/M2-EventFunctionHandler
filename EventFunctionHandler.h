/*
	Sherer 2017-2024 (C) - Revisioned by martysama0134 for c++20 support.
	Feel free to use it on your own.
	But ffs don`t remove this.
	Revisioned by martysama0134 for c++20 support.
*/

#pragma once
#include "utils.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

struct SArgumentSupportImpl {}; // keep for v1 compatibility

class CEventFunctionHandler : public singleton<CEventFunctionHandler>
{
	struct SFunctionHandler
	{
		std::function<void(SArgumentSupportImpl *)> func;
		size_t time{};
		size_t loopTime{};

		SFunctionHandler(std::function<void(SArgumentSupportImpl*)> _func, const size_t _time, const bool _loop = false)
			: func(std::move(_func)),
			  time(_time + get_global_time()),
			  loopTime(_loop ? _time : 0) {}

		void UpdateTime(const size_t newtime) { time = newtime + get_global_time(); }

		bool IsLooped() const { return loopTime != 0; }
		void UpdateNextLoopTime() { time = loopTime + get_global_time(); }
	};

public:
	CEventFunctionHandler() = default;
	virtual ~CEventFunctionHandler() = default; // Destroy() only clears up std containers so no need to explicitly call it here

	void Destroy();
	bool AddEvent(std::function<void(SArgumentSupportImpl*)> func, const std::string_view event_name, const size_t runtime, const bool loop = false);
	void RemoveEvent(const std::string_view event_name);
	void DelayEvent(const std::string_view event_name, const size_t newtime);
	bool FindEvent(const std::string_view event_name) const;
	DWORD GetDelay(const std::string_view event_name) const;
	void Process();

private:
	SFunctionHandler* GetHandlerByName(const std::string_view event_name) const;
	std::unordered_map<std::string, std::unique_ptr<SFunctionHandler>> m_event;
	bool bProcessStatus{true};
};

/*
	Sherer 2017-2024 (C) - Revisioned by martysama0134 for c++20 support.
	Feel free to use it on your own.
	But ffs don`t remove this.
*/

#include "stdafx.h"
#include "config.h"
#include "EventFunctionHandler.h"

void CEventFunctionHandler::Destroy()
{
	m_event.clear();
}

/*
	AddEvent(Function_to_Call, Event_Name, Time, Loop)
	Adding new event.

	Function_to_Call - std::function object containing function called when event appears.
	Event_Name - unique event name. If you accidently provide name of event which already exists, function returns false (if could rewrite current one but i did deny this idea).
	Time - execution time (in sec).
	Loop - repeat the event every Time seconds.
*/
bool CEventFunctionHandler::AddEvent(std::function<void(SArgumentSupportImpl *)> func, const std::string_view event_name, const size_t time, const bool loop)
{
	if (GetHandlerByName(event_name))
		return false;
	m_event.emplace(event_name, std::make_unique<SFunctionHandler>(std::move(func), time, loop));
	return true;
}

/*
	RemoveEvent(Event_Name)
	Deleting event if exists.

	Event_Name - unique event name.
*/
void CEventFunctionHandler::RemoveEvent(const std::string_view event_name)
{
	m_event_pending.emplace_back(event_name.data());
}

/*
	DelayEvent(Event_Name, NewTime)
	Changing existing event time. Disabled for looped events.

	Event_Name - unique event name.
	NewTime - new time.
*/
void CEventFunctionHandler::DelayEvent(const std::string_view event_name, const size_t newtime)
{
	if (auto ptr = GetHandlerByName(event_name); !ptr->IsLooped())
		ptr->UpdateTime(newtime);
}

/*
	FindEvent(Event_Name)
	Checking if event exists.

	Event_Name - unique event name.
*/
bool CEventFunctionHandler::FindEvent(const std::string_view event_name) const
{
	return GetHandlerByName(event_name) != nullptr;
}

/*
	GetDelay(Event_Name)
	Returning event delay.

	Event_Name - unique event name.
*/
DWORD CEventFunctionHandler::GetDelay(const std::string_view event_name) const
{
	if (const auto ptr = GetHandlerByName(event_name); ptr && ptr->time >= get_global_time())
		return ptr->time - get_global_time();
	return 0;
}

void CEventFunctionHandler::Process()
{
	if (m_event.empty())
		return;

	const auto current_time = get_global_time();
	std::erase_if(m_event, [&](const auto& pair) {
		const auto& [event_name, event_fnc] = pair;
		if (current_time >= event_fnc->time)
		{
			event_fnc->func(nullptr);
			if (event_fnc->IsLooped())
			{
				event_fnc->UpdateNextLoopTime();
				return false;
			}
			return true;
		}
		return false;
	});

	for (const auto& name : m_event_pending)
		m_event.erase(name);

	m_event_pending.clear();
}

CEventFunctionHandler::SFunctionHandler * CEventFunctionHandler::GetHandlerByName(const std::string_view event_name) const
{
	if (auto it = m_event.find(event_name.data()); it != m_event.end())
		return it->second.get();
	return nullptr;
}

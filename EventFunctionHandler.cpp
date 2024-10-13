/*
	Sherer 2017 (C)
	Feel free to use it on your own.
	But ffs don`t remove this.
*/

#include "stdafx.h"
#include "config.h"
#include "EventFunctionHandler.h"

CEventFunctionHandler::CEventFunctionHandler() : bProcessStatus(true)
{}

CEventFunctionHandler::~CEventFunctionHandler()
{
	Destroy();
}

void CEventFunctionHandler::Destroy()
{
	m_event.clear();
	bProcessStatus = false;
}

/*
	AddEvent(Function_to_Call, Event_Name, RunTime, SupportArg)
	Adding new event.

	Function_to_Call - std::function object containing function called when event appears. Keep in mind that functions always take one argument of type: SArgumentSupportImpl *.
	Event_Name - unique event name. If you accidently provide name of event which already exists, function returns false (if could rewrite current one but i did deny this idea).
	RunTime - execution time (in sec).
	SupportArg - argument for Function_to_Call: SArgumentSupportImpl *.
	
	SArgumentSupportImpl definition can be found in the header files.
*/
bool CEventFunctionHandler::AddEvent(std::function<void(SArgumentSupportImpl *)> func, const std::string & event_name, const size_t & runtime, SArgumentSupportImpl * support_arg)
{
	if (GetHandlerByName(event_name))
		return false;

	m_event.insert(std::make_pair(event_name, std::unique_ptr<SFunctionHandler>(new SFunctionHandler(func, runtime, support_arg))));
	return true;
}

/*
	RemoveEvent(Event_Name)
	Deleting event if exists.

	Event_Name - unique event name.
*/
void CEventFunctionHandler::RemoveEvent(const std::string & event_name)
{
	auto ptr = GetHandlerByName(event_name);
	if (ptr)
		m_event.erase(event_name);
}

/*
	DelayEvent(Event_Name, NewTime)
	Changing existing event time.

	Event_Name - unique event name.
	NewTime - new time.
*/
void CEventFunctionHandler::DelayEvent(const std::string & event_name, const size_t & newtime)
{
	auto ptr = GetHandlerByName(event_name);
	if (ptr)
		ptr->UpdateTime(newtime);
}

/*
	FindEvent(Event_Name)
	Checking if event exists.

	Event_Name - unique event name.
*/
bool CEventFunctionHandler::FindEvent(const std::string & event_name)
{
	auto ptr = GetHandlerByName(event_name);
	return (ptr != nullptr);
}

/*
	GetDelay(Event_Name)
	Returning event delay.

	Event_Name - unique event name.
*/
DWORD CEventFunctionHandler::GetDelay(const std::string & event_name)
{
	auto ptr = GetHandlerByName(event_name);
	if (ptr)
		return (ptr->time >= get_global_time()) ? (ptr->time-get_global_time()) : 0;
	else
		return 0;
}

void CEventFunctionHandler::Process()
{
	if (!bProcessStatus || !m_event.size())
		return;

	std::vector<std::string> v_delete;
	for (const auto & event : m_event)
	{
		if (get_global_time() >= (event.second).get()->time)
		{
			(event.second).get()->func((event.second).get()->SupportArg.get());
			v_delete.push_back(event.first);
		}
	}

	if (!v_delete.size())
		return;

	for (const auto & key : v_delete)
		m_event.erase(key);
}

CEventFunctionHandler::SFunctionHandler * CEventFunctionHandler::GetHandlerByName(const std::string & event_name)
{
	if (m_event.find(event_name) == m_event.end())
		return nullptr;

	return (m_event.find(event_name)->second).get();
}


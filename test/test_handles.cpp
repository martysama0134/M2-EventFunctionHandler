#include "doctest.h"
#include "config.h"
#include "EventFunctionHandler.h"

TEST_CASE("Default EventHandle is invalid")
{
	CEventFunctionHandler::EventHandle h{};
	CHECK_FALSE(h);
}

TEST_CASE("Valid handle from AddEvent")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddEvent([&]() -> int32_t { return 0; }, "valid", 5);
	CHECK(h);
}

TEST_CASE("Stale handle after RemoveEvent")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddEvent([&]() -> int32_t { return 0; }, "stale_rem", 5);
	handler.RemoveEvent(h);
	CHECK_FALSE(handler.FindEvent(h));
	CHECK_FALSE(handler.RemoveEvent(h));
	CHECK(handler.GetDelay(h) == 0);
	CHECK_FALSE(handler.DelayEvent(h, 10));
	CHECK_FALSE(handler.IsPulseEvent(h));
}

TEST_CASE("Stale handle after Destroy")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddEvent([&]() -> int32_t { return 0; }, "stale_destroy", 5);
	handler.Destroy();
	CHECK_FALSE(handler.FindEvent(h));
	CHECK_FALSE(handler.RemoveEvent(h));
}

TEST_CASE("Handle survives DelayEvent")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddEvent([&]() -> int32_t { return 0; }, "survive_delay", 5);
	handler.DelayEvent(h, 20);
	CHECK(handler.FindEvent(h));
	CHECK(handler.GetDelay(h) == 20);
}

TEST_CASE("Handle survives callback rescheduling")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddEvent([&]() -> int32_t { ++fired; return 5; }, "survive_resched", 3);
	set_global_time(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	CHECK(handler.GetDelay(h) > 0);
}

TEST_CASE("Handle survives pulse rescheduling")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddPulseEvent([&]() -> int32_t { ++fired; return 10; }, "survive_pulse", 5);
	set_pulse(5);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	CHECK(handler.GetPulseDelay(h) == 10);
}

TEST_CASE("Handle survives loop rescheduling")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddLoopEvent([&]() -> int32_t { ++fired; return 0; }, "survive_loop", 3);
	set_global_time(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
}

TEST_CASE("Handle vs string equivalence")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddEvent([&]() -> int32_t { return 0; }, "equiv", 10);
	CHECK(handler.FindEvent(h) == handler.FindEvent("equiv"));
	CHECK(handler.GetDelay(h) == handler.GetDelay("equiv"));
	CHECK(handler.IsPulseEvent(h) == handler.IsPulseEvent("equiv"));
	CHECK(handler.RemoveEvent(h));
	CHECK_FALSE(handler.FindEvent("equiv"));
}

TEST_CASE("Invalid handle on all operations returns false/0")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	CEventFunctionHandler::EventHandle bad{};
	CHECK_FALSE(handler.FindEvent(bad));
	CHECK_FALSE(handler.RemoveEvent(bad));
	CHECK_FALSE(handler.DelayEvent(bad, 5));
	CHECK(handler.GetDelay(bad) == 0);
	CHECK(handler.GetPulseDelay(bad) == 0);
	CHECK_FALSE(handler.IsPulseEvent(bad));
}

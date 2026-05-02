#include "doctest.h"
#include "config.h"
#include "EventFunctionHandler.h"

TEST_CASE("AddEvent one-shot fires at correct time")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddEvent([&]() -> int32_t { ++fired; return 0; }, "once", 5);
	REQUIRE(h);
	set_global_time(4);
	handler.Process();
	CHECK(fired == 0);
	set_global_time(5);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent("once"));
	CHECK_FALSE(handler.FindEvent(h));
}

TEST_CASE("AddEvent duplicate name rejected")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h1 = handler.AddEvent([&]() -> int32_t { return 0; }, "dup", 5);
	auto h2 = handler.AddEvent([&]() -> int32_t { return 0; }, "dup", 10);
	CHECK(h1);
	CHECK_FALSE(h2);
}

TEST_CASE("AddEvent time=0 fires on next Process")
{
	set_global_time(100);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddEvent([&]() -> int32_t { ++fired; return 0; }, "zero", 0);
	handler.Process();
	CHECK(fired == 1);
}

TEST_CASE("AddEvent empty name rejected")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddEvent([&]() -> int32_t { return 0; }, "", 5);
	CHECK_FALSE(h);
}

TEST_CASE("AddEvent null callback rejected")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddEvent(nullptr, "null_cb", 5);
	CHECK_FALSE(h);
}

TEST_CASE("AddEvent callback return > 0 reschedules")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddEvent([&]() -> int32_t { ++fired; return 3; }, "resched", 5);
	set_global_time(5);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	set_global_time(7);
	handler.Process();
	CHECK(fired == 1);
	set_global_time(8);
	handler.Process();
	CHECK(fired == 2);
}

TEST_CASE("AddLoopEvent repeats at interval")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddLoopEvent([&]() -> int32_t { ++fired; return 0; }, "loop", 3);
	REQUIRE(h);
	set_global_time(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	set_global_time(6);
	handler.Process();
	CHECK(fired == 2);
	CHECK(handler.FindEvent(h));
}

TEST_CASE("AddLoopEvent interval=0 rejected")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddLoopEvent([&]() -> int32_t { return 0; }, "bad_loop", 0);
	CHECK_FALSE(h);
}

TEST_CASE("AddLoopEvent callback return > 0 overrides interval")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddLoopEvent([&]() -> int32_t { ++fired; return (fired == 1) ? 10 : 0; }, "override_loop", 3);
	set_global_time(3);
	handler.Process();
	CHECK(fired == 1);
	set_global_time(6);
	handler.Process();
	CHECK(fired == 1);
	set_global_time(13);
	handler.Process();
	CHECK(fired == 2);
}

TEST_CASE("AddLoopEvent stopped by RemoveEvent from callback")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddLoopEvent([&]() -> int32_t {
		++fired;
		handler.RemoveEvent("stopper");
		return 0;
	}, "stopper", 3);
	set_global_time(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent("stopper"));
	set_global_time(6);
	handler.Process();
	CHECK(fired == 1);
}

TEST_CASE("RemoveEvent returns true/false")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddEvent([&]() -> int32_t { return 0; }, "rem", 5);
	CHECK(handler.RemoveEvent(h));
	CHECK_FALSE(handler.RemoveEvent(h));
	CHECK_FALSE(handler.RemoveEvent("nonexistent"));
}

TEST_CASE("DelayEvent changes timing")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddEvent([&]() -> int32_t { ++fired; return 0; }, "delay", 5);
	CHECK(handler.DelayEvent(h, 10));
	CHECK(handler.GetDelay(h) == 10);
	set_global_time(5);
	handler.Process();
	CHECK(fired == 0);
	set_global_time(10);
	handler.Process();
	CHECK(fired == 1);
}

TEST_CASE("DelayEvent returns false for pulse/looped/missing")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	handler.AddPulseEvent([&]() -> int32_t { return 0; }, "p", 10);
	handler.AddLoopEvent([&]() -> int32_t { return 0; }, "l", 5);
	CHECK_FALSE(handler.DelayEvent("p", 20));
	CHECK_FALSE(handler.DelayEvent("l", 20));
	CHECK_FALSE(handler.DelayEvent("missing", 20));
}

TEST_CASE("Multiple events expire at same tick")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int a = 0, b = 0, c = 0;
	handler.AddEvent([&]() -> int32_t { ++a; return 0; }, "a", 5);
	handler.AddEvent([&]() -> int32_t { ++b; return 0; }, "b", 5);
	handler.AddEvent([&]() -> int32_t { ++c; return 0; }, "c", 5);
	set_global_time(5);
	handler.Process();
	CHECK(a == 1);
	CHECK(b == 1);
	CHECK(c == 1);
}

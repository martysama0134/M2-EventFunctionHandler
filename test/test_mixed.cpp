#include "doctest.h"
#include "config.h"
#include "EventFunctionHandler.h"
#include <vector>

TEST_CASE("Seconds + pulse coexist, fire independently")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int sec = 0, pul = 0;
	handler.AddEvent([&]() -> int32_t { ++sec; return 0; }, "mix_s", 5);
	handler.AddPulseEvent([&]() -> int32_t { ++pul; return 0; }, "mix_p", 10);
	set_global_time(5);
	set_pulse(5);
	handler.Process();
	CHECK(sec == 1);
	CHECK(pul == 0);
	set_pulse(10);
	handler.Process();
	CHECK(pul == 1);
}

TEST_CASE("Seconds fire before pulses in same Process")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	std::vector<int> order;
	handler.AddPulseEvent([&]() -> int32_t { order.push_back(2); return 0; }, "order_p", 5);
	handler.AddEvent([&]() -> int32_t { order.push_back(1); return 0; }, "order_s", 5);
	set_global_time(5);
	set_pulse(5);
	handler.Process();
	REQUIRE(order.size() == 2);
	CHECK(order[0] == 1);
	CHECK(order[1] == 2);
}

TEST_CASE("Shared namespace collision")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	handler.AddEvent([&]() -> int32_t { return 0; }, "shared", 5);
	CHECK_FALSE(handler.AddPulseEvent([&]() -> int32_t { return 0; }, "shared", 10));
}

TEST_CASE("Empty name rejected by all Add functions")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	CHECK_FALSE(handler.AddEvent([&]() -> int32_t { return 0; }, "", 5));
	CHECK_FALSE(handler.AddPulseEvent([&]() -> int32_t { return 0; }, "", 5));
	CHECK_FALSE(handler.AddLoopEvent([&]() -> int32_t { return 0; }, "", 5));
}

TEST_CASE("Null callback rejected by all Add functions")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	CHECK_FALSE(handler.AddEvent(nullptr, "n1", 5));
	CHECK_FALSE(handler.AddPulseEvent(nullptr, "n2", 5));
	CHECK_FALSE(handler.AddLoopEvent(nullptr, "n3", 5));
}

TEST_CASE("Callback return overflow deactivates event")
{
	set_global_time(SIZE_MAX - 5);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddEvent([&]() -> int32_t { ++fired; return INT32_MAX; }, "overflow", 1);
	set_global_time(SIZE_MAX - 4);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent("overflow"));
}

TEST_CASE("DelayEvent overflow rejected")
{
	set_global_time(SIZE_MAX - 5);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddEvent([&]() -> int32_t { return 0; }, "delay_ovf", 1);
	CHECK_FALSE(handler.DelayEvent(h, SIZE_MAX));
}

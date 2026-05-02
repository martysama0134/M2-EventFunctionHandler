#include "doctest.h"
#include "config.h"
#include "EventFunctionHandler.h"

TEST_CASE("GetDelay returns remaining seconds")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddEvent([&]() -> int32_t { return 0; }, "gd", 10);
	CHECK(handler.GetDelay(h) == 10);
	CHECK(handler.GetDelay("gd") == 10);
	set_global_time(3);
	CHECK(handler.GetDelay(h) == 7);
}

TEST_CASE("GetDelay returns 0 for pulse events")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	handler.AddPulseEvent([&]() -> int32_t { return 0; }, "gd_pulse", 10);
	CHECK(handler.GetDelay("gd_pulse") == 0);
}

TEST_CASE("GetDelay returns 0 for missing/expired")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	CHECK(handler.GetDelay("nonexistent") == 0);
	CEventFunctionHandler::EventHandle invalid{};
	CHECK(handler.GetDelay(invalid) == 0);
}

TEST_CASE("GetPulseDelay returns remaining pulses")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h = handler.AddPulseEvent([&]() -> int32_t { return 0; }, "gpd", 10);
	CHECK(handler.GetPulseDelay(h) == 10);
	CHECK(handler.GetPulseDelay("gpd") == 10);
	set_pulse(3);
	CHECK(handler.GetPulseDelay(h) == 7);
}

TEST_CASE("GetPulseDelay returns 0 for seconds/missing")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	handler.AddEvent([&]() -> int32_t { return 0; }, "sec", 10);
	CHECK(handler.GetPulseDelay("sec") == 0);
	CHECK(handler.GetPulseDelay("nonexistent") == 0);
}

TEST_CASE("IsPulseEvent")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto hp = handler.AddPulseEvent([&]() -> int32_t { return 0; }, "ipe_p", 10);
	auto hs = handler.AddEvent([&]() -> int32_t { return 0; }, "ipe_s", 10);
	CHECK(handler.IsPulseEvent(hp));
	CHECK(handler.IsPulseEvent("ipe_p"));
	CHECK_FALSE(handler.IsPulseEvent(hs));
	CHECK_FALSE(handler.IsPulseEvent("ipe_s"));
	CHECK_FALSE(handler.IsPulseEvent("nonexistent"));
}

TEST_CASE("IsPulseEvent after name reuse")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	handler.AddPulseEvent([&]() -> int32_t { return 0; }, "reuse", 10);
	handler.RemoveEvent("reuse");
	handler.AddEvent([&]() -> int32_t { return 0; }, "reuse", 10);
	CHECK_FALSE(handler.IsPulseEvent("reuse"));
}

TEST_CASE("FindEvent works for both types")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	handler.AddPulseEvent([&]() -> int32_t { return 0; }, "fe_p", 10);
	handler.AddEvent([&]() -> int32_t { return 0; }, "fe_s", 10);
	CHECK(handler.FindEvent("fe_p"));
	CHECK(handler.FindEvent("fe_s"));
}

TEST_CASE("DelayEvent silently ignores pulse events")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	handler.AddPulseEvent([&]() -> int32_t { return 0; }, "de_p", 10);
	CHECK_FALSE(handler.DelayEvent("de_p", 100));
	CHECK(handler.GetPulseDelay("de_p") == 10);
}

TEST_CASE("Same-name collision across types")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	handler.AddEvent([&]() -> int32_t { return 0; }, "shared", 5);
	auto h = handler.AddPulseEvent([&]() -> int32_t { return 0; }, "shared", 10);
	CHECK_FALSE(h);
}

#include "doctest.h"
#include "config.h"
#include "EventFunctionHandler.h"

TEST_CASE("AddPulseEvent fires at correct pulse")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddPulseEvent([&]() -> int32_t { ++fired; return 0; }, "p_once", 10);
	REQUIRE(h);
	set_pulse(9);
	handler.Process();
	CHECK(fired == 0);
	set_pulse(10);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent(h));
}

TEST_CASE("Pulse return > 0 reschedules")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddPulseEvent([&]() -> int32_t { ++fired; return 5; }, "p_resched", 10);
	set_pulse(10);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	set_pulse(14);
	handler.Process();
	CHECK(fired == 1);
	set_pulse(15);
	handler.Process();
	CHECK(fired == 2);
}

TEST_CASE("Pulse dynamic delay per invocation")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int count = 0;
	handler.AddPulseEvent([&]() -> int32_t {
		++count;
		if (count == 1) return 10;
		if (count == 2) return 5;
		return 0;
	}, "p_dyn", 3);
	set_pulse(3);
	handler.Process();
	CHECK(count == 1);
	set_pulse(13);
	handler.Process();
	CHECK(count == 2);
	set_pulse(18);
	handler.Process();
	CHECK(count == 3);
	CHECK_FALSE(handler.FindEvent("p_dyn"));
}

TEST_CASE("Pulse off-by-one: delay 1 fires at N+1")
{
	set_global_time(0);
	set_pulse(100);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddPulseEvent([&]() -> int32_t { ++fired; return 0; }, "p_obo", 1);
	handler.Process();
	CHECK(fired == 0);
	set_pulse(101);
	handler.Process();
	CHECK(fired == 1);
}

TEST_CASE("Pulse delay<=0 clamped to 1")
{
	set_global_time(0);
	set_pulse(50);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddPulseEvent([&]() -> int32_t { ++fired; return 0; }, "p_clamp", 0);
	REQUIRE(h);
	handler.Process();
	CHECK(fired == 0);
	set_pulse(51);
	handler.Process();
	CHECK(fired == 1);
}

TEST_CASE("Pulse negative delay clamped to 1")
{
	set_global_time(0);
	set_pulse(50);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddPulseEvent([&]() -> int32_t { ++fired; return 0; }, "p_neg", -5);
	handler.Process();
	CHECK(fired == 0);
	set_pulse(51);
	handler.Process();
	CHECK(fired == 1);
}

TEST_CASE("Pulse callback returns negative — stops")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddPulseEvent([&]() -> int32_t { ++fired; return -1; }, "p_neg_ret", 3);
	set_pulse(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent("p_neg_ret"));
}

TEST_CASE("Multiple pulse events same tick")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int a = 0, b = 0, c = 0;
	handler.AddPulseEvent([&]() -> int32_t { ++a; return 0; }, "mp1", 5);
	handler.AddPulseEvent([&]() -> int32_t { ++b; return 0; }, "mp2", 5);
	handler.AddPulseEvent([&]() -> int32_t { ++c; return 0; }, "mp3", 5);
	set_pulse(5);
	handler.Process();
	CHECK(a == 1);
	CHECK(b == 1);
	CHECK(c == 1);
}

TEST_CASE("Duplicate pulse name rejected")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto h1 = handler.AddPulseEvent([&]() -> int32_t { return 0; }, "dup_p", 5);
	auto h2 = handler.AddPulseEvent([&]() -> int32_t { return 0; }, "dup_p", 10);
	CHECK(h1);
	CHECK_FALSE(h2);
}

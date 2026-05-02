#include "doctest.h"
#include "config.h"
#include "EventFunctionHandler.h"
#include <memory>

TEST_CASE("v2 compat: SArgumentSupportImpl* void callback fires")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddEvent([&](SArgumentSupportImpl*) { ++fired; }, "v2_void", 3);
	REQUIRE(h);
	set_global_time(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent(h));
}

TEST_CASE("v2 compat: SArgumentSupportImpl* with loop=true repeats")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddEvent([&](SArgumentSupportImpl*) { ++fired; }, "v2_loop", 2, true);
	REQUIRE(h);
	set_global_time(2);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	set_global_time(4);
	handler.Process();
	CHECK(fired == 2);
	CHECK(handler.FindEvent(h));
}

TEST_CASE("v2 compat: SArgumentSupportImpl* loop with self-removal stops")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddEvent([&](SArgumentSupportImpl*) {
		++fired;
		if (fired >= 3)
			handler.RemoveEvent("v2_selfrem");
	}, "v2_selfrem", 1, true);
	set_global_time(1);
	handler.Process();
	set_global_time(2);
	handler.Process();
	set_global_time(3);
	handler.Process();
	CHECK(fired == 3);
	CHECK_FALSE(handler.FindEvent("v2_selfrem"));
	set_global_time(4);
	handler.Process();
	CHECK(fired == 3);
}

TEST_CASE("v2 compat: SArgumentSupportImpl* pulse callback")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddPulseEvent([&](SArgumentSupportImpl*) -> int32_t { ++fired; return 5; }, "v2_pulse", 10);
	REQUIRE(h);
	set_pulse(10);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	set_pulse(15);
	handler.Process();
	CHECK(fired == 2);
}

TEST_CASE("v4 void callback: no return fires as one-shot")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddEvent([&]() { ++fired; }, "void_oneshot", 3);
	REQUIRE(h);
	set_global_time(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent(h));
}

TEST_CASE("v4 void callback: loop repeats")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddLoopEvent([&]() { ++fired; }, "void_loop", 2);
	REQUIRE(h);
	set_global_time(2);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	set_global_time(4);
	handler.Process();
	CHECK(fired == 2);
}

TEST_CASE("v4 void pulse callback: fires once")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddPulseEvent([&]() { ++fired; }, "void_pulse", 5);
	REQUIRE(h);
	set_pulse(5);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent(h));
}

TEST_CASE("Null std::function rejected")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	std::function<void(SArgumentSupportImpl*)> null_v2;
	CHECK_FALSE(handler.AddEvent(null_v2, "null_v2", 5));
	std::function<int32_t()> null_v4;
	CHECK_FALSE(handler.AddEvent(null_v4, "null_v4", 5));
	std::function<void()> null_void;
	CHECK_FALSE(handler.AddEvent(null_void, "null_void", 5));
}

TEST_CASE("v4 int32_t callback: return > 0 reschedules")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddEvent([&]() -> int32_t { ++fired; return (fired < 3) ? 2 : 0; }, "int32_resched", 1);
	REQUIRE(h);
	set_global_time(1);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	set_global_time(3);
	handler.Process();
	CHECK(fired == 2);
	set_global_time(5);
	handler.Process();
	CHECK(fired == 3);
	CHECK_FALSE(handler.FindEvent(h));
}

TEST_CASE("v4 int32_t pulse callback: return > 0 reschedules")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddPulseEvent([&]() -> int32_t { ++fired; return 5; }, "int32_pulse", 10);
	REQUIRE(h);
	set_pulse(10);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	set_pulse(15);
	handler.Process();
	CHECK(fired == 2);
}

TEST_CASE("Nullptr function pointer rejected")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int32_t (*null_fp)() = nullptr;
	CHECK_FALSE(handler.AddEvent(null_fp, "null_fp", 5));
}

TEST_CASE("v2 compat: AddLoopEvent with SArgumentSupportImpl*")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddLoopEvent([&](SArgumentSupportImpl*) { ++fired; }, "v2_addloop", 3);
	REQUIRE(h);
	set_global_time(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK(handler.FindEvent(h));
	set_global_time(6);
	handler.Process();
	CHECK(fired == 2);
}

TEST_CASE("v2 compat: AddEvent with explicit loop=false")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	auto h = handler.AddEvent([&](SArgumentSupportImpl*) { ++fired; }, "v2_explicit_false", 3, false);
	REQUIRE(h);
	set_global_time(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent(h));
}

TEST_CASE("ReleaseId clears captures immediately")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto obj = std::make_shared<int>(42);
	std::weak_ptr<int> weak = obj;
	handler.AddEvent([obj]() -> int32_t { return 0; }, "capture_release", 5);
	obj.reset();
	CHECK_FALSE(weak.expired());
	handler.RemoveEvent("capture_release");
	CHECK(weak.expired());
}

TEST_CASE("Null v2 std::function rejected")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	std::function<int32_t(SArgumentSupportImpl*)> null_v2_int;
	CHECK_FALSE(handler.AddEvent(null_v2_int, "null_v2_int", 5));
}

TEST_CASE("v2 compat: captures work correctly")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	auto counter = std::make_shared<int>(0);
	handler.AddEvent([counter](SArgumentSupportImpl*) {
		++(*counter);
	}, "v2_capture", 3);
	set_global_time(3);
	handler.Process();
	CHECK(*counter == 1);
}

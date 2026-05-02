#include "doctest.h"
#include "config.h"
#include "EventFunctionHandler.h"

TEST_CASE("Pulse self-remove from callback — no reschedule")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddPulseEvent([&]() -> int32_t {
		++fired;
		handler.RemoveEvent("p_selfrem");
		return 5;
	}, "p_selfrem", 3);
	set_pulse(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent("p_selfrem"));
	set_pulse(100);
	handler.Process();
	CHECK(fired == 1);
}

TEST_CASE("Re-add same name from inside callback")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int orig = 0, added = 0;
	handler.AddPulseEvent([&]() -> int32_t {
		++orig;
		handler.RemoveEvent("readd");
		handler.AddPulseEvent([&]() -> int32_t { ++added; return 0; }, "readd", 5);
		return 0;
	}, "readd", 3);
	set_pulse(3);
	handler.Process();
	CHECK(orig == 1);
	CHECK(added == 0);
	CHECK(handler.FindEvent("readd"));
	set_pulse(8);
	handler.Process();
	CHECK(added == 1);
}

TEST_CASE("Cross-removal: earlier callback removes later event — skipped via pre-fire")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int victim = 0, killer = 0;
	handler.AddPulseEvent([&]() -> int32_t { ++victim; return 0; }, "victim", 10);
	handler.AddPulseEvent([&]() -> int32_t {
		++killer;
		handler.RemoveEvent("victim");
		return 0;
	}, "killer", 5);
	set_pulse(5);
	handler.Process();
	CHECK(killer == 1);
	CHECK_FALSE(handler.FindEvent("victim"));
	set_pulse(10);
	handler.Process();
	CHECK(victim == 0);
}

TEST_CASE("Destroy during callback — remaining skip, new adds to fresh state")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int cb1 = 0, cb2 = 0, post = 0;
	handler.AddEvent([&]() -> int32_t {
		++cb1;
		handler.Destroy();
		handler.AddEvent([&]() -> int32_t { ++post; return 0; }, "post_destroy", 5);
		return 0;
	}, "destroyer", 3);
	handler.AddPulseEvent([&]() -> int32_t { ++cb2; return 5; }, "victim2", 3);
	set_global_time(3);
	set_pulse(3);
	handler.Process();
	CHECK(cb1 == 1);
	CHECK(cb2 == 1);
	CHECK(handler.FindEvent("post_destroy"));
	CHECK_FALSE(handler.FindEvent("destroyer"));
	CHECK_FALSE(handler.FindEvent("victim2"));
	set_global_time(8);
	handler.Process();
	CHECK(post == 1);
}

TEST_CASE("Callback adds pulse event with delay 0 (clamped) — doesn't fire this tick")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int child = 0;
	handler.AddPulseEvent([&]() -> int32_t {
		handler.AddPulseEvent([&]() -> int32_t { ++child; return 0; }, "child", 0);
		return 0;
	}, "parent", 5);
	set_pulse(5);
	handler.Process();
	CHECK(child == 0);
	CHECK(handler.FindEvent("child"));
	set_pulse(6);
	handler.Process();
	CHECK(child == 1);
}

TEST_CASE("Recursive Process is no-op")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddEvent([&]() -> int32_t {
		++fired;
		handler.Process();
		return 0;
	}, "recurse", 1);
	set_global_time(1);
	handler.Process();
	CHECK(fired == 1);
}

TEST_CASE("Re-entrant AddEvent doesn't fire this tick")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int orig = 0, added = 0;
	handler.AddEvent([&]() -> int32_t {
		++orig;
		handler.AddEvent([&]() -> int32_t { ++added; return 0; }, "added_during", 3);
		return 0;
	}, "trigger", 2);
	set_global_time(2);
	handler.Process();
	CHECK(orig == 1);
	CHECK(added == 0);
	set_global_time(5);
	handler.Process();
	CHECK(added == 1);
}

TEST_CASE("Remove + re-add same name — old heap entry stale")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int old_fired = 0, new_fired = 0;
	handler.AddPulseEvent([&]() -> int32_t { ++old_fired; return 0; }, "stale", 5);
	handler.RemoveEvent("stale");
	handler.AddPulseEvent([&]() -> int32_t { ++new_fired; return 0; }, "stale", 10);
	set_pulse(5);
	handler.Process();
	CHECK(old_fired == 0);
	CHECK(new_fired == 0);
	set_pulse(10);
	handler.Process();
	CHECK(new_fired == 1);
}

TEST_CASE("External RemoveEvent on pending pulse")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddPulseEvent([&]() -> int32_t { ++fired; return 0; }, "ext_rem", 5);
	handler.RemoveEvent("ext_rem");
	set_pulse(5);
	handler.Process();
	CHECK(fired == 0);
}

TEST_CASE("Looped seconds self-remove stops loop")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	int fired = 0;
	handler.AddLoopEvent([&]() -> int32_t {
		++fired;
		handler.RemoveEvent("loop_selfrem");
		return 0;
	}, "loop_selfrem", 3);
	set_global_time(3);
	handler.Process();
	CHECK(fired == 1);
	CHECK_FALSE(handler.FindEvent("loop_selfrem"));
	set_global_time(6);
	handler.Process();
	CHECK(fired == 1);
}

TEST_CASE("Destroy clears all including queues")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;
	handler.AddPulseEvent([&]() -> int32_t { return 5; }, "dp", 3);
	handler.AddEvent([&]() -> int32_t { return 0; }, "ds", 3);
	handler.Destroy();
	CHECK_FALSE(handler.FindEvent("dp"));
	CHECK_FALSE(handler.FindEvent("ds"));
}

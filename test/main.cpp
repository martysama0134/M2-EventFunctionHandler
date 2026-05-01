#include "EventFunctionHandler.h"
#include "config.h"
#include <cassert>
#include <string>

int main()
{
	set_global_time(0);
	CEventFunctionHandler handler;

	// AddEvent succeeds, duplicate returns false
	int fired_once = 0;
	assert(handler.AddEvent([&](SArgumentSupportImpl*) { ++fired_once; }, "once_event", 5));
	assert(!handler.AddEvent([](SArgumentSupportImpl*) {}, "once_event", 1));

	// FindEvent
	assert(handler.FindEvent("once_event"));
	assert(!handler.FindEvent("nonexistent"));

	// GetDelay returns remaining time
	assert(handler.GetDelay("once_event") == 5);

	// DelayEvent changes timing
	handler.DelayEvent("once_event", 10);
	assert(handler.GetDelay("once_event") == 10);

	// Process does not fire before time
	set_global_time(9);
	handler.Process();
	assert(fired_once == 0);
	assert(handler.FindEvent("once_event"));

	// Process fires at correct time and removes non-looped event
	set_global_time(10);
	handler.Process();
	assert(fired_once == 1);
	assert(!handler.FindEvent("once_event"));

	// RemoveEvent prevents firing
	int fired_removed = 0;
	set_global_time(0);
	assert(handler.AddEvent([&](SArgumentSupportImpl*) { ++fired_removed; }, "removed_event", 5));
	handler.RemoveEvent("removed_event");
	assert(!handler.FindEvent("removed_event"));
	set_global_time(100);
	handler.Process();
	assert(fired_removed == 0);

	// Looped event survives Process and fires again
	int fired_loop = 0;
	set_global_time(0);
	assert(handler.AddEvent([&](SArgumentSupportImpl*) { ++fired_loop; }, "loop_event", 3, true));

	set_global_time(3);
	handler.Process();
	assert(fired_loop == 1);
	assert(handler.FindEvent("loop_event"));

	set_global_time(6);
	handler.Process();
	assert(fired_loop == 2);
	assert(handler.FindEvent("loop_event"));

	// DelayEvent has no effect on looped events
	handler.DelayEvent("loop_event", 100);
	assert(handler.GetDelay("loop_event") == 3);

	// Multiple events expiring at same tick
	handler.Destroy();
	int fired_a = 0, fired_b = 0, fired_c = 0;
	set_global_time(0);
	assert(handler.AddEvent([&](SArgumentSupportImpl*) { ++fired_a; }, "multi_a", 5));
	assert(handler.AddEvent([&](SArgumentSupportImpl*) { ++fired_b; }, "multi_b", 5));
	assert(handler.AddEvent([&](SArgumentSupportImpl*) { ++fired_c; }, "multi_c", 5));
	set_global_time(5);
	handler.Process();
	assert(fired_a == 1);
	assert(fired_b == 1);
	assert(fired_c == 1);
	assert(!handler.FindEvent("multi_a"));
	assert(!handler.FindEvent("multi_b"));
	assert(!handler.FindEvent("multi_c"));

	// Re-entrant callback: callback adds event during Process
	handler.Destroy();
	int fired_original = 0, fired_added = 0;
	set_global_time(0);
	assert(handler.AddEvent([&](SArgumentSupportImpl*) {
		++fired_original;
		handler.AddEvent([&](SArgumentSupportImpl*) { ++fired_added; }, "added_during_process", 3);
	}, "reentrant_add", 2));
	set_global_time(2);
	handler.Process();
	assert(fired_original == 1);
	assert(handler.FindEvent("added_during_process"));
	assert(fired_added == 0);
	set_global_time(5);
	handler.Process();
	assert(fired_added == 1);

	// Re-entrant callback: callback removes event during Process
	handler.Destroy();
	int fired_survivor = 0, fired_remover = 0;
	set_global_time(0);
	assert(handler.AddEvent([&](SArgumentSupportImpl*) { ++fired_survivor; }, "survivor", 5));
	assert(handler.AddEvent([&](SArgumentSupportImpl*) {
		++fired_remover;
		handler.RemoveEvent("survivor");
	}, "remover", 3));
	set_global_time(3);
	handler.Process();
	assert(fired_remover == 1);
	assert(!handler.FindEvent("survivor"));
	set_global_time(10);
	handler.Process();
	assert(fired_survivor == 0);

	// Destroy clears all
	handler.Destroy();
	assert(!handler.FindEvent("remover"));

	// === PULSE BASICS ===

	// AddPulseEvent fires at correct pulse
	handler.Destroy();
	int fired_pulse = 0;
	set_global_time(0);
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_pulse; return 0; }, "pulse_once", 10));
	set_pulse(9);
	handler.Process();
	assert(fired_pulse == 0);
	set_pulse(10);
	handler.Process();
	assert(fired_pulse == 1);
	assert(!handler.FindEvent("pulse_once"));

	// Return > 0 reschedules
	handler.Destroy();
	int fired_resched = 0;
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_resched; return 5; }, "pulse_resched", 10));
	set_pulse(10);
	handler.Process();
	assert(fired_resched == 1);
	assert(handler.FindEvent("pulse_resched"));
	set_pulse(14);
	handler.Process();
	assert(fired_resched == 1);
	set_pulse(15);
	handler.Process();
	assert(fired_resched == 2);

	// Dynamic delay — different return each invocation
	handler.Destroy();
	int dynamic_count = 0;
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long {
		++dynamic_count;
		if (dynamic_count == 1) return 10;
		if (dynamic_count == 2) return 5;
		return 0;
	}, "pulse_dynamic", 3));
	set_pulse(3);
	handler.Process();
	assert(dynamic_count == 1);
	set_pulse(13);
	handler.Process();
	assert(dynamic_count == 2);
	set_pulse(18);
	handler.Process();
	assert(dynamic_count == 3);
	assert(!handler.FindEvent("pulse_dynamic"));

	// Off-by-one: delay 1 fires at N+1
	handler.Destroy();
	int fired_offbyone = 0;
	set_pulse(100);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_offbyone; return 0; }, "pulse_obo", 1));
	handler.Process();
	assert(fired_offbyone == 0);
	set_pulse(101);
	handler.Process();
	assert(fired_offbyone == 1);

	// === PULSE SAFETY ===

	// Pulse self-remove from callback
	handler.Destroy();
	int fired_selfrem = 0;
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long {
		++fired_selfrem;
		handler.RemoveEvent("pulse_selfrem");
		return 5;
	}, "pulse_selfrem", 3));
	set_pulse(3);
	handler.Process();
	assert(fired_selfrem == 1);
	assert(!handler.FindEvent("pulse_selfrem"));
	set_pulse(100);
	handler.Process();
	assert(fired_selfrem == 1);

	// Re-add same name from inside pulse callback
	handler.Destroy();
	int fired_readd_orig = 0, fired_readd_new = 0;
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long {
		++fired_readd_orig;
		handler.RemoveEvent("pulse_readd");
		handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_readd_new; return 0; }, "pulse_readd", 5);
		return 0;
	}, "pulse_readd", 3));
	set_pulse(3);
	handler.Process();
	assert(fired_readd_orig == 1);
	assert(handler.FindEvent("pulse_readd"));
	assert(fired_readd_new == 0);
	set_pulse(8);
	handler.Process();
	assert(fired_readd_new == 1);

	// Callback adds pulse event with delay 0 (clamped to 1)
	handler.Destroy();
	int fired_delay0 = 0;
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long {
		handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_delay0; return 0; }, "pulse_delay0_child", 0);
		return 0;
	}, "pulse_delay0_parent", 5));
	set_pulse(5);
	handler.Process();
	assert(fired_delay0 == 0);
	assert(handler.FindEvent("pulse_delay0_child"));
	set_pulse(6);
	handler.Process();
	assert(fired_delay0 == 1);

	// Callback removes a different pulse event
	handler.Destroy();
	int fired_victim = 0, fired_killer = 0;
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_victim; return 0; }, "pulse_victim", 10));
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long {
		++fired_killer;
		handler.RemoveEvent("pulse_victim");
		return 0;
	}, "pulse_killer", 5));
	set_pulse(5);
	handler.Process();
	assert(fired_killer == 1);
	assert(!handler.FindEvent("pulse_victim"));
	set_pulse(10);
	handler.Process();
	assert(fired_victim == 0);

	// === QUERY API ===

	// GetDelay returns 0 for pulse events
	handler.Destroy();
	set_global_time(0);
	set_pulse(0);
	assert(handler.AddPulseEvent([](SArgumentSupportImpl*) -> long { return 0; }, "pulse_gd", 10));
	assert(handler.GetDelay("pulse_gd") == 0);

	// GetPulseDelay returns raw pulses remaining
	assert(handler.GetPulseDelay("pulse_gd") == 10);
	set_pulse(3);
	assert(handler.GetPulseDelay("pulse_gd") == 7);

	// GetPulseDelay returns 0 for seconds events
	assert(handler.AddEvent([](SArgumentSupportImpl*) {}, "sec_gpd", 10));
	assert(handler.GetPulseDelay("sec_gpd") == 0);

	// GetPulseDelay returns 0 for missing/removed events
	assert(handler.GetPulseDelay("nonexistent") == 0);
	handler.RemoveEvent("pulse_gd");
	assert(handler.GetPulseDelay("pulse_gd") == 0);

	// IsPulseEvent
	handler.Destroy();
	set_pulse(0);
	assert(handler.AddPulseEvent([](SArgumentSupportImpl*) -> long { return 0; }, "pulse_ipe", 10));
	assert(handler.AddEvent([](SArgumentSupportImpl*) {}, "sec_ipe", 10));
	assert(handler.IsPulseEvent("pulse_ipe"));
	assert(!handler.IsPulseEvent("sec_ipe"));
	assert(!handler.IsPulseEvent("nonexistent"));

	// IsPulseEvent after name reuse (pulse removed, seconds added with same name)
	handler.RemoveEvent("pulse_ipe");
	assert(!handler.IsPulseEvent("pulse_ipe"));
	assert(handler.AddEvent([](SArgumentSupportImpl*) {}, "pulse_ipe", 10));
	assert(!handler.IsPulseEvent("pulse_ipe"));

	// FindEvent works for both types
	handler.Destroy();
	set_pulse(0);
	assert(handler.AddPulseEvent([](SArgumentSupportImpl*) -> long { return 0; }, "pulse_fe", 10));
	assert(handler.AddEvent([](SArgumentSupportImpl*) {}, "sec_fe", 10));
	assert(handler.FindEvent("pulse_fe"));
	assert(handler.FindEvent("sec_fe"));

	// DelayEvent silently ignores pulse events
	handler.Destroy();
	set_global_time(0);
	set_pulse(0);
	assert(handler.AddPulseEvent([](SArgumentSupportImpl*) -> long { return 0; }, "pulse_de", 10));
	handler.DelayEvent("pulse_de", 100);
	assert(handler.GetPulseDelay("pulse_de") == 10);

	// === MIXED ===

	// Seconds + pulse coexist, fire independently
	handler.Destroy();
	int fired_sec_mix = 0, fired_pulse_mix = 0;
	set_global_time(0);
	set_pulse(0);
	assert(handler.AddEvent([&](SArgumentSupportImpl*) { ++fired_sec_mix; }, "sec_mix", 5));
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_pulse_mix; return 0; }, "pulse_mix", 10));
	set_global_time(5);
	set_pulse(5);
	handler.Process();
	assert(fired_sec_mix == 1);
	assert(fired_pulse_mix == 0);
	set_pulse(10);
	handler.Process();
	assert(fired_pulse_mix == 1);

	// Same-name collision (shared namespace)
	handler.Destroy();
	set_pulse(0);
	assert(handler.AddEvent([](SArgumentSupportImpl*) {}, "shared_name", 5));
	assert(!handler.AddPulseEvent([](SArgumentSupportImpl*) -> long { return 0; }, "shared_name", 10));

	// Seconds fire before pulses within same Process()
	handler.Destroy();
	std::vector<int> order;
	set_global_time(0);
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { order.push_back(2); return 0; }, "order_pulse", 5));
	assert(handler.AddEvent([&](SArgumentSupportImpl*) { order.push_back(1); }, "order_sec", 5));
	set_global_time(5);
	set_pulse(5);
	handler.Process();
	assert(order.size() == 2);
	assert(order[0] == 1);
	assert(order[1] == 2);

	// === EDGE CASES ===

	// AddEvent with time=0 fires on next Process
	handler.Destroy();
	int fired_time0 = 0;
	set_global_time(100);
	assert(handler.AddEvent([&](SArgumentSupportImpl*) { ++fired_time0; }, "time0_event", 0));
	handler.Process();
	assert(fired_time0 == 1);

	// AddPulseEvent with delay=0 clamped to 1
	handler.Destroy();
	int fired_pd0 = 0;
	set_pulse(50);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_pd0; return 0; }, "pd0_event", 0));
	handler.Process();
	assert(fired_pd0 == 0);
	set_pulse(51);
	handler.Process();
	assert(fired_pd0 == 1);

	// Multiple pulse events same tick
	handler.Destroy();
	int fired_mp1 = 0, fired_mp2 = 0, fired_mp3 = 0;
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_mp1; return 0; }, "mp1", 5));
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_mp2; return 0; }, "mp2", 5));
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_mp3; return 0; }, "mp3", 5));
	set_pulse(5);
	handler.Process();
	assert(fired_mp1 == 1);
	assert(fired_mp2 == 1);
	assert(fired_mp3 == 1);

	// Callback returns negative — treated as stop
	handler.Destroy();
	int fired_neg = 0;
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_neg; return -1; }, "pulse_neg", 3));
	set_pulse(3);
	handler.Process();
	assert(fired_neg == 1);
	assert(!handler.FindEvent("pulse_neg"));

	// Duplicate pulse event names rejected
	handler.Destroy();
	set_pulse(0);
	assert(handler.AddPulseEvent([](SArgumentSupportImpl*) -> long { return 0; }, "dup_pulse", 5));
	assert(!handler.AddPulseEvent([](SArgumentSupportImpl*) -> long { return 0; }, "dup_pulse", 10));

	// === STALE / LIFECYCLE ===

	// Remove + re-add same name — old heap entry doesn't fire new event
	handler.Destroy();
	int fired_old = 0, fired_new = 0;
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_old; return 0; }, "stale_test", 5));
	handler.RemoveEvent("stale_test");
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_new; return 0; }, "stale_test", 10));
	set_pulse(5);
	handler.Process();
	assert(fired_old == 0);
	assert(fired_new == 0);
	set_pulse(10);
	handler.Process();
	assert(fired_old == 0);
	assert(fired_new == 1);

	// Looped seconds callback removes itself
	handler.Destroy();
	int fired_loop_selfrem = 0;
	set_global_time(0);
	assert(handler.AddEvent([&](SArgumentSupportImpl*) {
		++fired_loop_selfrem;
		handler.RemoveEvent("loop_selfrem");
	}, "loop_selfrem", 3, true));
	set_global_time(3);
	handler.Process();
	assert(fired_loop_selfrem == 1);
	assert(!handler.FindEvent("loop_selfrem"));
	set_global_time(6);
	handler.Process();
	assert(fired_loop_selfrem == 1);

	// Destroy clears all structures including queues
	handler.Destroy();
	set_pulse(0);
	assert(handler.AddPulseEvent([](SArgumentSupportImpl*) -> long { return 5; }, "destroy_p", 3));
	assert(handler.AddEvent([](SArgumentSupportImpl*) {}, "destroy_s", 3));
	handler.Destroy();
	assert(!handler.FindEvent("destroy_p"));
	assert(!handler.FindEvent("destroy_s"));

	// External RemoveEvent on pending pulse event
	handler.Destroy();
	int fired_ext_rem = 0;
	set_pulse(0);
	assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++fired_ext_rem; return 0; }, "ext_rem_pulse", 5));
	handler.RemoveEvent("ext_rem_pulse");
	set_pulse(5);
	handler.Process();
	assert(fired_ext_rem == 0);

	// === SCALE ===

	// 1000+ events with mixed adds, removes, reschedules
	handler.Destroy();
	set_global_time(0);
	set_pulse(0);

	constexpr int SCALE_COUNT = 1200;
	int scale_sec_fired = 0;
	int scale_pulse_fired = 0;

	for (int i = 0; i < SCALE_COUNT; ++i)
	{
		const std::string sec_name = "scale_sec_" + std::to_string(i);
		const std::string pulse_name = "scale_pulse_" + std::to_string(i);
		assert(handler.AddEvent([&](SArgumentSupportImpl*) { ++scale_sec_fired; }, sec_name, 10));
		assert(handler.AddPulseEvent([&](SArgumentSupportImpl*) -> long { ++scale_pulse_fired; return 0; }, pulse_name, 25));
	}

	// Remove every other one
	for (int i = 0; i < SCALE_COUNT; i += 2)
	{
		handler.RemoveEvent("scale_sec_" + std::to_string(i));
		handler.RemoveEvent("scale_pulse_" + std::to_string(i));
	}

	set_global_time(10);
	set_pulse(25);
	handler.Process();

	assert(scale_sec_fired == SCALE_COUNT / 2);
	assert(scale_pulse_fired == SCALE_COUNT / 2);

	// All should be gone now (one-shot)
	for (int i = 0; i < SCALE_COUNT; ++i)
	{
		assert(!handler.FindEvent("scale_sec_" + std::to_string(i)));
		assert(!handler.FindEvent("scale_pulse_" + std::to_string(i)));
	}

	handler.Destroy();

	return 0;
}


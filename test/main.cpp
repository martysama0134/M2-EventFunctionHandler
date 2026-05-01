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

	return 0;
}

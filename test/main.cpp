#include "EventFunctionHandler.h"
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

	return 0;
}

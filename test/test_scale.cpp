#include "doctest.h"
#include "config.h"
#include "EventFunctionHandler.h"
#include <string>

TEST_CASE("10k events with mixed adds and removes")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;

	constexpr int COUNT = 10000;
	int sec_fired = 0;
	int pulse_fired = 0;

	for (int i = 0; i < COUNT; ++i)
	{
		handler.AddEvent([&]() -> int32_t { ++sec_fired; return 0; }, "s_" + std::to_string(i), 10);
		handler.AddPulseEvent([&]() -> int32_t { ++pulse_fired; return 0; }, "p_" + std::to_string(i), 25);
	}

	for (int i = 0; i < COUNT; i += 2)
	{
		handler.RemoveEvent("s_" + std::to_string(i));
		handler.RemoveEvent("p_" + std::to_string(i));
	}

	set_global_time(10);
	set_pulse(25);
	handler.Process();

	CHECK(sec_fired == COUNT / 2);
	CHECK(pulse_fired == COUNT / 2);

	for (int i = 0; i < COUNT; ++i)
	{
		CHECK_FALSE(handler.FindEvent("s_" + std::to_string(i)));
		CHECK_FALSE(handler.FindEvent("p_" + std::to_string(i)));
	}

	handler.Destroy();
}

TEST_CASE("Stale heap stress — 5k add/remove far-future events")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;

	for (int i = 0; i < 5000; ++i)
	{
		handler.AddEvent([&]() -> int32_t { return 0; }, "stale_" + std::to_string(i), 999999);
		handler.RemoveEvent("stale_" + std::to_string(i));
	}

	handler.AddEvent([&]() -> int32_t { return 0; }, "survivor", 1);
	set_global_time(1);
	handler.Process();
	CHECK_FALSE(handler.FindEvent("survivor"));

	handler.Destroy();
}

TEST_CASE("10k handles remain valid through operations")
{
	set_global_time(0);
	set_pulse(0);
	CEventFunctionHandler handler;

	constexpr int COUNT = 10000;
	std::vector<CEventFunctionHandler::EventHandle> handles;
	handles.reserve(COUNT);

	for (int i = 0; i < COUNT; ++i)
		handles.push_back(handler.AddEvent([&]() -> int32_t { return 0; }, "h_" + std::to_string(i), 100));

	for (int i = 0; i < COUNT; i += 3)
		handler.RemoveEvent(handles[i]);

	int valid = 0, invalid = 0;
	for (int i = 0; i < COUNT; ++i)
	{
		if (handler.FindEvent(handles[i]))
			++valid;
		else
			++invalid;
	}

	CHECK(valid > 0);
	CHECK(invalid > 0);
	CHECK(valid + invalid == COUNT);

	handler.Destroy();
}

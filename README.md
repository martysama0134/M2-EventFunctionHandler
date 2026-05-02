# M2-EventFunctionHandler

[![CI](https://github.com/martysama0134/M2-EventFunctionHandler/actions/workflows/ci.yml/badge.svg)](https://github.com/martysama0134/M2-EventFunctionHandler/actions/workflows/ci.yml)

A C++20 event scheduler for Metin2 game servers. Supports seconds-based timers and sub-second pulse-based events with priority-queue internals for efficient scheduling at scale.

```txt
#####################################################################################
# ORIGINAL TOPIC: https://metin2.dev/topic/17432-reimplementation-of-events/        #
# Created by Sherer 2017-2024 (C) - Revisioned by martysama0134 for c++20 support.  #
#####################################################################################
```

## Features

- **Opaque EventHandle** — type-safe handle with generation-based stale detection
- **Seconds-based events** — one-shot, looped (`AddLoopEvent`), or callback-rescheduled
- **Pulse-based events** — sub-second scheduling driven by server tick (e.g. 25 Hz)
- **10k+ event scale** — integer-ID internals, dual priority queues, O(log n) scheduling
- **Safe re-entrancy** — collect-then-fire execution model prevents UB on self/cross-removal
- **Unified callback** — `std::function<int32_t()>`, return > 0 to reschedule
- **v2 backward compatible** — old `SArgumentSupportImpl*` callbacks compile without changes

## Integration

1) Add include into `main.cpp`:
```cpp
#ifdef __NEW_EVENT_HANDLER__
#include "EventFunctionHandler.h"
#endif
```

2) Search for:
```cpp
	while (idle());
```

And add before:
```cpp
	#ifdef __NEW_EVENT_HANDLER__
	CEventFunctionHandler EventFunctionHandler;
	#endif
```

3) Add at the end of `idle`:
```cpp
	#ifdef __NEW_EVENT_HANDLER__
	CEventFunctionHandler::instance().Process();
	#endif
```

4) Search for:
```cpp
	sys_log(0, "<shutdown> Destroying CArenaManager...");
```

And add before:
```cpp
	#ifdef __NEW_EVENT_HANDLER__
	sys_log(0, "<shutdown> Destroying CEventFunctionHandler...");
	CEventFunctionHandler::instance().Destroy();
	#endif
```

5) Open `service.h` and add:
```cpp
#define __NEW_EVENT_HANDLER__
```

## Download

Download [master.zip](../../archive/refs/heads/main.zip), and add the included files to your source.

## Usage

### Seconds-Based Events

Callbacks can be `void` (one-shot) or return `int32_t` (return `0` to stop, `> 0` to reschedule after that many seconds).

```cpp
auto& handler = CEventFunctionHandler::instance();

// One-shot event — void lambda, fires once after 5 seconds
auto h = handler.AddEvent([this]() {
		this->SendNotificationToAll();
	}, "MY_BEAUTIFUL_EVENT", std::chrono::seconds(5).count()
);

// One-shot with rescheduling — return > 0 to repeat
handler.AddEvent([this]() -> int32_t {
		this->Tick();
		return 3; // reschedule after 3 seconds
	}, "MY_TICKER", 5
);

// Looped event — repeats every 5 minutes
handler.AddLoopEvent([this]() {
		this->SendNotificationToAll();
	}, "MY_LOOP_EVENT", std::chrono::minutes(5).count()
);

// Delay a non-looped event by 5s (works with handle or name)
handler.DelayEvent(h, std::chrono::seconds(5).count());
handler.DelayEvent("MY_BEAUTIFUL_EVENT", std::chrono::seconds(5).count());

// Get remaining time (seconds). Returns 0 if not found.
size_t remaining = handler.GetDelay(h);

// Cancel the event. Returns true if found and removed.
bool removed = handler.RemoveEvent(h);
```

### Pulse-Based Events

Pulse events run on the server tick (typically 25 Hz). Void callbacks fire once. Return `> 0` to reschedule after that many pulses, `<= 0` to stop.

```cpp
auto& handler = CEventFunctionHandler::instance();

// One-shot pulse event — void lambda, fires once after 25 pulses
auto h = handler.AddPulseEvent([this]() {
		this->UpdatePosition();
	}, "POSITION_UPDATE", passes_per_sec
);

// Repeating pulse event with dynamic delay
handler.AddPulseEvent([this]() -> int32_t {
		this->ProcessAI();
		return passes_per_sec / 5; // reschedule every 200ms
	}, "AI_TICK", passes_per_sec / 5
);

// Check remaining pulses
size_t pulses = handler.GetPulseDelay("AI_TICK");

// Check if an event is pulse-based
bool isPulse = handler.IsPulseEvent("AI_TICK");
```

### EventHandle

All `Add*` methods return an `EventHandle` — an opaque token for O(1) lookups. Handles become stale after the event is removed or destroyed.

```cpp
auto h = handler.AddEvent([&]() -> int32_t { return 0; }, "MY_EVENT", 5);

if (h)                       // valid handle (operator bool)
    handler.FindEvent(h);    // true — O(1) lookup
handler.RemoveEvent(h);
handler.FindEvent(h);        // false — handle is stale

// All query/modify methods accept both EventHandle and string_view
handler.FindEvent("MY_EVENT");     // equivalent string lookup
```

### v2 Backward Compatibility

Existing v2 code compiles without changes. The `SArgumentSupportImpl*` parameter is accepted and ignored:

```cpp
// v2 code — works unchanged on v4
CEventFunctionHandler::instance().AddEvent([this](SArgumentSupportImpl*) {
		this->BroadcastAlert();
	}, "MY_EVENT", cooldown
);

// v2 looped event — also works unchanged
CEventFunctionHandler::instance().AddEvent([this](SArgumentSupportImpl*) {
		this->DoStuff();
	}, "MY_LOOP", 1, true
);
```

### API Reference

| Method | Returns | Description |
|---|---|---|
| `AddEvent(func, name, time, loop)` | `EventHandle` | Seconds event. Void callback = one-shot. Return > 0 to reschedule. `loop=true` for fixed-interval repeat. |
| `AddLoopEvent(func, name, interval)` | `EventHandle` | Looped seconds event. Repeats at interval unless callback returns > 0. |
| `AddPulseEvent(func, name, pulseDelay)` | `EventHandle` | Pulse event. Callback returns next delay (> 0 reschedule, <= 0 stop). |
| `RemoveEvent(handle\|name)` | `bool` | Remove event. Returns true if found. |
| `FindEvent(handle\|name)` | `bool` | Check if event exists. |
| `DelayEvent(handle\|name, newtime)` | `bool` | Reschedule seconds non-looped event. Returns false for pulse/looped. |
| `GetDelay(handle\|name)` | `size_t` | Remaining seconds. 0 for pulse/missing. |
| `GetPulseDelay(handle\|name)` | `size_t` | Remaining pulses. 0 for seconds/missing. |
| `IsPulseEvent(handle\|name)` | `bool` | True if active pulse event. |
| `Destroy()` | `void` | Clear all events and internal state. |
| `Process()` | `void` | Fire due events. Call once per server tick. |

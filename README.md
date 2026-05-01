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

- **Seconds-based events** — one-shot or looped, with delay/reschedule support
- **Pulse-based events** — sub-second scheduling driven by server tick (e.g. 25 Hz)
- **10k+ event scale** — integer-ID internals, dual priority queues, O(log n) scheduling
- **Safe re-entrancy** — collect-then-fire execution model prevents UB on self/cross-removal
- **Zero API breakage** — existing `AddEvent` signatures unchanged

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

```cpp
// One-shot event — fires once after 5 seconds
CEventFunctionHandler::instance().AddEvent([this](SArgumentSupportImpl*) {
		this->SendNotificationToAll();
	}, "MY_BEAUTIFUL_EVENT", std::chrono::seconds(5).count()
);

// Looped event — repeats every 5 minutes
CEventFunctionHandler::instance().AddEvent([this](SArgumentSupportImpl*) {
		this->SendNotificationToAll();
	}, "MY_BEAUTIFUL_EVENT", std::chrono::minutes(5).count(), true
);

// Delay a non-looped event by 5s
if (CEventFunctionHandler::instance().FindEvent("MY_BEAUTIFUL_EVENT"))
	CEventFunctionHandler::instance().DelayEvent("MY_BEAUTIFUL_EVENT", std::chrono::seconds(5).count());

// Get remaining time (seconds). Returns 0 if expired or not found.
DWORD remaining = CEventFunctionHandler::instance().GetDelay("MY_BEAUTIFUL_EVENT");

// Cancel the event. Safe even if the key doesn't exist.
CEventFunctionHandler::instance().RemoveEvent("MY_BEAUTIFUL_EVENT");
```

### Pulse-Based Events

Pulse events run on the server tick (typically 25 Hz). The callback returns the next delay in pulses (`> 0` to reschedule, `<= 0` to stop).

```cpp
// One-shot pulse event — fires after 25 pulses (1 second at 25 Hz)
CEventFunctionHandler::instance().AddPulseEvent([this](SArgumentSupportImpl*) -> int32_t {
		this->UpdatePosition();
		return 0; // don't reschedule
	}, "POSITION_UPDATE", passes_per_sec
);

// Repeating pulse event with dynamic delay
CEventFunctionHandler::instance().AddPulseEvent([this](SArgumentSupportImpl*) -> int32_t {
		this->ProcessAI();
		return passes_per_sec / 5; // reschedule every 200ms
	}, "AI_TICK", passes_per_sec / 5
);

// Check remaining pulses
DWORD pulses = CEventFunctionHandler::instance().GetPulseDelay("AI_TICK");

// Check if an event is pulse-based
bool isPulse = CEventFunctionHandler::instance().IsPulseEvent("AI_TICK");
```

### API Reference

| Method | Description |
|---|---|
| `AddEvent(func, name, time, loop)` | Add seconds-based event. Returns false if name exists. |
| `AddPulseEvent(func, name, pulseDelay)` | Add pulse-based event. Callback returns next delay. |
| `RemoveEvent(name)` | Remove any event by name. Safe if not found. |
| `FindEvent(name)` | Check if event exists. Works for both types. |
| `DelayEvent(name, newtime)` | Reschedule seconds non-looped event. Ignores pulse/looped. |
| `GetDelay(name)` | Remaining seconds. Returns 0 for pulse/missing events. |
| `GetPulseDelay(name)` | Remaining pulses. Returns 0 for seconds/missing events. |
| `IsPulseEvent(name)` | True if active pulse event. |
| `Destroy()` | Clear all events and internal state. |
| `Process()` | Fire due events. Call once per server tick. |

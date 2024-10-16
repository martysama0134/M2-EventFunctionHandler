
### Intro

```txt
#####################################################################################
# ORIGINAL TOPIC: https://metin2.dev/topic/17432-reimplementation-of-events/        #
# Created by Sherer 2017-2024 (C) - Revisioned by martysama0134 for c++20 support.  #
#####################################################################################
```


### Implementation
1) Add include into the `main.cpp`:
	```cpp
	#ifdef __NEW_EVENT_HANDLER__
	#include "EventFunctionHandler.h"
	#endif
	```

2) Now search for:
	```cpp
		while (idle());
	```

	And add before:
	```cpp
		#ifdef __NEW_EVENT_HANDLER__
		CEventFunctionHandler EventFunctionHandler;
		#endif
	```


3) Now add this at the end of idle:
	```cpp
		#ifdef __NEW_EVENT_HANDLER__
		CEventFunctionHandler::instance().Process();
		#endif
	```



4) Now search for:
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


5) Now open `service.h` and add this define:
	```cpp
	#define __NEW_EVENT_HANDLER__
	```


### Download
Download [master.zip](../../archive/refs/heads/main.zip), and add the included files to your source.


### Usage

```cpp
	// Create the event and call it after 5 seconds. Using "this" as argument is safe only if it's a singleton, for CHARACTER or CItem, use their vid and find them inside the lambda.
	CEventFunctionHandler::instance().AddEvent([this](SArgumentSupportImpl*) {
			this->SendNotificationToAll();
		}, "MY_BEAUTIFUL_EVENT", std::chrono::seconds(5).count()
	);

	// Create the event and loop it every 5 minutes. Don't forget, keys already existing will be skipped, such as this one.
	CEventFunctionHandler::instance().AddEvent([this](SArgumentSupportImpl*) {
			this->SendNotificationToAll();
		}, "MY_BEAUTIFUL_EVENT", std::chrono::minutes(5).count(), true
	);

	// Check if it exists, then delay it again by 5s. Don't forget, DelayEvent has no effect to looped events.
	if (CEventFunctionHandler::instance().FindEvent("MY_BEAUTIFUL_EVENT"))
		CEventFunctionHandler::Instance().DelayEvent("MY_BEAUTIFUL_EVENT", std::chrono::seconds(5).count());

	// Cancel the event. Safe even if the key doesn't exist.
	CEventFunctionHandler::Instance().RemoveEvent("MY_BEAUTIFUL_EVENT");
```

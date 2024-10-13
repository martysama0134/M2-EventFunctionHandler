```txt
#####################################################################################
# ORIGINAL TOPIC: https://metin2.dev/topic/17432-reimplementation-of-events/        #
# Created by Sherer 2017-2024 (C) - Revisioned by martysama0134 for c++20 support.  #
#####################################################################################
```

Hello,

Working on some new stuff I found out that current implementation of event looks a bit tricky.

Due to this fact I basically deciced to re-implement it providing up to date tech.

So lets begin.

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

That`s all.

Now just download [master.zip](../../archive/refs/heads/main.zip) and add the included files to your source.

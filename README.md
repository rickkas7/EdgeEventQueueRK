# EdgeEventQueueRK

*Store and forward publishing queue for Tracker Edge and Monitor Edge*

NOTE: You should use the official version of this library instead! It can be found at:

https://github.com/particle-iot/EdgeEventQueue

---

- Repository: https://github.com/rickkas7/EdgeEventQueueRK
- License: MIT (free for use in open or closed-source projects, including commercial projects)
- [Browseable API docs](https://rickkas7.github.io/EdgeEventQueueRK/index.html)

This library is intended to be used with Tracker Edge or Monitor Edge for implementing custom store and forward queueing. Because its requires the
function of your Edge software, there are no library examples in this repository as they wouldn't be able to be compiled successfully.

For more information about store and forward, see the [Particle docs](https://docs.particle.io/firmware/tracker-edge/sotre-and-forward/).


## Setup

### Tracker Edge


- Add the EdgeEventQueueRK library to your Tracker Edge product, typically using **Particle: Install Library** in Particle Workbench.

- Add the include file:

```cpp
#include "EdgeEventQueueRK.h"
```

- Add a global variable for your queue, typically in main.cpp. You will typically one have one, but you can have multiple queues.

```cpp
static EdgeEventQueueRK privateEventQueue;
```

- Initialize the library from `setup()`:

```cpp
privateEventQueue
    .withSizeLimit(50 * 1024)
    .withQueuePath("/usr/testq")
    .setup();
```

- Make sure you provide time to handle the queue by adding a call to `loop()`:

```cpp
privateEventQueue.loop();
```


### Monitor Edge

- Add the EdgeEventQueueRK library to your Monitor Edge product, typically using **Particle: Install Library** in Particle Workbench.

- Add the include file:

```cpp
#include "EdgeEventQueueRK.h"
```

- Add a global variable for your queue, typically in user_setup.cpp. You will typically one have one, but you can have multiple queues.

```cpp
static EdgeEventQueueRK privateEventQueue;
```

- Initialize the library from `user_init()`:

```cpp
privateEventQueue
    .withSizeLimit(50 * 1024)
    .withQueuePath("/usr/testq")
    .setup();
```

- Make sure you provide time to handle the queue by adding a call to `user_loop()`:

```cpp
privateEventQueue.loop();
```

### Setup options

At minimum you will probably want to set a queue path and call setup, as in the examples above. You chain together as many of the
`withXXX()` options you want, then finally call `.setup()` to complete the setup.

```cpp
privateEventQueue
    .withQueuePath("/usr/testq")
    .setup();
```

#### withPriority

```cpp
// PROTOTYPE
EdgeEventQueueRK &withPriority(size_t priority)
```

- priority 0 or 1. 0 is the high priority queue and 1 is the normal priority queue. Location publishes use the normal queue, 1. The default is 1.

#### withPublishFlags

Since `PRIVATE` is always used now (there is no `PUBLIC`), the only flag that applies now is `NO_ACK`, however you will
rarely use this with queued events.

```cpp 
// PROTOTYPE
EdgeEventQueueRK &withPublishFlags(PublishFlags flags)
```

#### withSizeLimit

Set the size limit in bytes. Default is unlimited (0). There is no guarantee you will be able to save the limit you specify
as the space is not reserved, but it is treated as a maximum.

```cpp
// PROTOTYPE
EdgeEventQueueRK &withSizeLimit(size_t sizeLimit)
```

#### withDiskQueuePolicy

```cpp
// PROTOTYPE
EdgeEventQueueRK &withDiskQueuePolicy(DiskQueuePolicy policy)
```

- policy DiskQueuePolicy::FifoDeleteOld (default) or DiskQueuePolicy::FifoDeleteNew

#### withQueuePath

Set the directory path in the flash file system to store the queue files. Each event is stored in a separate file
in the queue directory. Each queue must have its own separate directory.

```cpp
// PROTOTYPE
EdgeEventQueueRK &withQueuePath(const char *path)
```

- path Typically put in "/usr/" directory. Does not need to exist; will be created if it does not exist.


## Using the library

### Publishing using the queue

To queue the data on the flash file system, use the `publish()` method.

```cpp
// PROTOTYPE
int publish(const char *eventName, const char *eventData);

// EXAMPLE
privateEventQueue.publish("eventQueueTest", eventData);
```

It returns 0 on success, or a non-zero error code. You will get a success result even if offline, as long as the event can be enqueued.

### Publishing without queueing

Sometimes you will want to publish an event without using the queue, because the event is temporal and historical data is not useful if the 
device is currently offline.

To do this, use `EdgeEventQueueRK::cloudServicePublish`, which takes an eventName and eventData. 

```cpp
EdgeEventQueueRK::cloudServicePublish("eventQueueTest", eventData);
```

This is preferable to directly using Particle.publish because it will interleave the emptying of the queue with sending your non-queued 
message and will not exceed the publish rate limit. 

The full API is:

```cpp
// PROTOTYPE - EdgeEventQueueRK
static int cloudServicePublish(const char *eventName, const char *eventData, PublishFlags publishFlags = {}, size_t priority = 0, std::function<int(CloudServiceStatus)> cb = 0);
```

- `eventName` The event name, as is used in `Particle.publish`. 

- `eventData` The event data, as is used in `Particle.publish`.

- `publishFlags` Publish flags, as is used in Particle.publish. This is optional, and if omitted the default flags are used.

- `priority` 0 or 1. 0 is the default queue and 1 is the low priority queue.

- `cb` Callback function to be called on successful completion or error. Optional. Not called if an immediate error
results in a non-zero result code; callback is only called if the return value is 0.

- Returns `int` 0 on success or a non-zero error code

The callback function has this prototype:

```cpp
int callback(CloudServiceStatus status)
```

- `status` is `particle::Error::NONE` (0) or an system error code on error

Callback is a std::function so you can pass a lambda, which allows you to pass additional data via capture variables, or
call a C++ class method and instance easily.

The eventName and eventValue are copied and do not need to remain valid until the callback is called. Once the
cloudServicePublish call returns, the variables can go out of scope, so it's safe for them to be local variables
on the stack.

Using cloudServicePublish interleaves your event with others in the system in a queue in RAM. The queue is finite
in size (currently 8 elements per priority queue) and if the queue is full, -EBUSY (-16) is returned.

Note that this function does not use the disk queue! It's a low-level function used by the publish method in this
class, or you can use it for your own purposes if you want to publish events that are not saved to disk if the device
is currently offline.


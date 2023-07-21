#ifndef __EDGEEVENTQUEUERK_H
#define __EDGEEVENTQUEUERK_H

#include "Particle.h"

#include "DiskQueue.h"
#include "cloud_service.h"


/**
 * The CloudService has a different prototype on Tracker Edge and Monitor Edge. These
 * defines can be used to determine which Edge firmware you are using. This class
 * defines a common API so you don't have to worry about it from your code.
 */
#if defined __has_include
#  if __has_include ("tracker.h")
#    define IS_TRACKER_EDGE 1
#    define IS_MONITOR_EDGE 0
#  else
#    define IS_TRACKER_EDGE 0
#    define IS_MONITOR_EDGE 1
#  endif
#endif

/**
 * @brief Class for managing a private queue of events on the flash file system
 * 
 * Only available on devices using TrackerEdge or MonitorEdge, as it utilizes the DiskQueue 
 * and CloudService classes. 
 * 
 * When metering out events to stay within the rate limit, this interleaves your private
 * events and the system events so you won't exceed the rate limit.
 * 
 * A queue size limit specified in this class is independent of the one set in the
 * cloud configuration. There is no checking done to make sure there's enough disk
 * space available across all queues.
 */
class EdgeEventQueueRK {
public: 
    /**
     * @brief Constructor
     * 
     * Construct one of these object for each queue. Often there will only be one, but the
     * code supports multiple queues. It is typically constructed as a global object.
     */
    EdgeEventQueueRK();

    /**
     * @brief Destructor 
     * 
     * This is typically not used as the object is typically instantiated as a global object.
     */
    virtual ~EdgeEventQueueRK();

    /**
     * @brief Call during setup(), the main application setup function
     * 
     * @return 0 on success or a non-zero error code
     * 
     * Typically used as: EdgeEventQueueRK::instance().setup();
     * 
     * Call the withXXX() methods to set configuration parameters before setup!
     * 
     */
    int setup();

    /**
     * @brief Call during loop(), the main application loop function
     * 
     * Typically used as: EdgeEventQueueRK::instance().loop();
     */
    void loop();

    /**
     * @brief Sets the priority to use for publishing. Default is 0.
     * 
     * @param priority 0 or 1. 0 is the default queue and 1 is the low priority queue.
     * 
     * @return EdgeEventQueueRK& Reference to this object to chain, fluent-style
     */
    EdgeEventQueueRK &withPriority(size_t priority) { this->priority = priority; return *this; };

    /**
     * @brief Sets the publish flags such as NO_ACK.
     * 
     * @param flags 
     * @return EdgeEventQueueRK& 
     */
    EdgeEventQueueRK &withPublishFlags(PublishFlags flags) { this->publishFlags = publishFlags; return *this; };



    /**
     * @brief Set the disk queue size limit (in bytes). Default is 0 (not limited).
     * 
     * @return EdgeEventQueueRK& Reference to this object to chain, fluent-style
     */
    EdgeEventQueueRK &withSizeLimit(size_t sizeLimit) { this->sizeLimit = sizeLimit; return *this; };

    /**
     * @brief Sets the disk queue policy for deleting events when the queue is full
     * 
     * @param policy DiskQueuePolicy::FifoDeleteOld (default) or DiskQueuePolicy::FifoDeleteNew
     * 
     * @return EdgeEventQueueRK& Reference to this object to chain, fluent-style
     */
    EdgeEventQueueRK &withDiskQueuePolicy(DiskQueuePolicy policy) { this->policy = policy; return *this; };

    /**
     * @brief Sets the queue path. Default is "/usr/privateq". Typically put in "/usr/" directory.
     * 
     * @param path Typically put in "/usr/" directory. Does not need to exist; will be created if it does not exist.
     * 
     * @return EdgeEventQueueRK& Reference to this object to chain, fluent-style
     */
    EdgeEventQueueRK &withQueuePath(const char *path) { this->path = path; return *this; };


    /**
     * @brief Add an event to the publish queue on the flash file system
     * 
     * @param eventName The event name, as is used in Particle.publish
     * 
     * @param eventData The event data, as is used in Particle.publish
     * 
     * @return int 0 on success or a non-zero error code
     * 
     * The eventName and eventValue are copied and do not need to remain valid until the callback is called. Once the
     * publish call returns, the variables can go out of scope, so it's safe for them to be local variables
     * on the stack.
     */
    int publish(const char *eventName, const char *eventData);


    /**
     * @brief Publishes an event using the cloud service without using the disk queue
     * 
     * @param eventName The event name, as is used in Particle.publish. 
     * 
     * @param eventData The event data, as is used in Particle.publish
     * 
     * @param publishFlags Publish flags, as is used in Particle.publish. This is optional, and if omitted the default flags are used.
     * 
     * @param priority 0 or 1. 0 is the default queue and 1 is the low priority queue.
     * 
     * @param cb Callback function to be called on successful completion or error. Optional. Not called if an immediate error
     * results in a non-zero result code; callback is only called if the return value is 0.
     * 
     * @return int 0 on success or a non-zero error code
     * 
     * The callback function has this prototype:
     * 
     * int callback(CloudServiceStatus status)
     * 
     * - `status` is `particle::Error::NONE` (0) or an system error code on error
     * 
     * It is a std::function so you can pass a lambda, which allows you to pass additional data via capture variables, or
     * call a C++ class method and instance easily.
     * 
     * The eventName and eventValue are copied and do not need to remain valid until the callback is called. Once the
     * cloudServicePublish call returns, the variables can go out of scope, so it's safe for them to be local variables
     * on the stack.
     * 
     * Using cloudServicePublish interleaves your event with others in the system in a queue in RAM. The queue is finite
     * in size (currently 8 elements per priority queue) and if the queue is full, -EBUSY (-16) is returned.
     * 
     * Note that this function does not use the disk queue! It's a low-level function used by the publish method in this
     * class, or you can use it for your own purposes if you want to publish events that are not saved to disk if the device
     * is currently offline.
     */
    static int cloudServicePublish(const char *eventName, const char *eventData, PublishFlags publishFlags = {}, size_t priority = 0, std::function<int(CloudServiceStatus)> cb = 0);

private:

    /**
     * @brief This class is not copyable
     */
    EdgeEventQueueRK(const EdgeEventQueueRK&) = delete;

    /**
     * @brief This class is not copyable
     */
    EdgeEventQueueRK& operator=(const EdgeEventQueueRK&) = delete;

    /**
     * @brief How long to wait after a successful publish
     * 
     * Can be 0 because the there's another queue that implements event rate limiting.
     */
    std::chrono::milliseconds successDelay = 1s;

    /**
     * @brief How long to wait before retrying after an error sending
     */
    std::chrono::milliseconds sendErrorDelay = 10s;

    /**
     * @brief If an immediate error occurs when attempting to publish, how long to wait before trying again
     * 
     * This typically occurs in the BackgroundPublish queue is full.
     */
    std::chrono::milliseconds immediateErrorDelay = 5s;

    /**
     * @brief If the publish callback is not called in this amount of time, how long to wait before trying again
     */
    std::chrono::milliseconds safetyCheckDelay = 60s;
    
    /**
     * @brief When to check to send again. 0 = ASAP, otherwise a System.millis() value (64-bit, does not roll over)
     */
    uint64_t nextCheck = 0;

    /**
     * @brief Which queue to use 0 = normal, 1 = low-priority
     */
    size_t priority = 0;

    /**
     * @brief Flags as for Particle.publish
     * 
     * Since there are no public events anymore, PRIVATE is always the case and does not need to be
     * specified. NO_ACK is one possibility if you don't want events acknowledged, though this could
     * lead to some lost events.
     */
    PublishFlags publishFlags;

    /**
     * @brief The DiskQueue object that manages queued events
     */
    DiskQueue diskQueue;

    /**
     * @brief Disk queue policy. Default: DiskQueuePolicy::FifoDeleteOld
     */
    DiskQueuePolicy policy = DiskQueuePolicy::FifoDeleteOld;

    /**
     * @brief Disk queue size limit (in bytes). Default is 0 (not limited).
     */
    size_t sizeLimit = 0;


    /**
     * @brief Sets the path to use for the queue
     */
    String path = "/usr/privateq";

};

#endif // __EDGEEVENTQUEUERK_H

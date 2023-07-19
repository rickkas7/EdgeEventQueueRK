#ifndef __EDGEEVENTQUEUERK_H
#define __EDGEEVENTQUEUERK_H

#include "Particle.h"

#include "DiskQueue.h"

class EdgeEventQueueRK {
public: 
    /**
     * @brief Get the singleton instance of the private disk queue object
     * 
     * @return EdgeEventQueueRK& Reference to the class instance
     */
    static EdgeEventQueueRK &instance();

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
     * @brief Add an event to the publish queue
     * 
     * @param eventName The event name, as is used in Particle.publish
     * 
     * @param eventData The event data, as is used in Particle.publish
     * 
     * @return int 0 on success or a non-zero error code
     * 
     * Note that the event name is not a publish parameter. It's expected to be the same for all events in the queue
     * and set using withEventName(). Likewise with withPublishFlags(), withPriority(), etc.
     */
    int publish(const char *eventName, const char *eventData);


private:
    /**
     * @brief Constructor (protected)
     * 
     * You never construct one of these - use the singleton instance using `EdgeEventQueueRK::instance()`.
     */
    EdgeEventQueueRK();

    /**
     * @brief Destructor - never used
     * 
     * The singleton cannot be deleted.
     */
    virtual ~EdgeEventQueueRK();

    /**
     * @brief This class is not copyable
     */
    EdgeEventQueueRK(const EdgeEventQueueRK&) = delete;

    /**
     * @brief This class is not copyable
     */
    EdgeEventQueueRK& operator=(const EdgeEventQueueRK&) = delete;

    std::chrono::milliseconds successDelay = 1s;
    std::chrono::milliseconds sendErrorDelay = 10s;
    std::chrono::milliseconds immediateErrorDelay = 5s;
    std::chrono::milliseconds safetyCheckDelay = 60s;
    

    uint64_t nextCheck = 0;

    size_t priority = 0;


    PublishFlags publishFlags;

    DiskQueue diskQueue;

    /**
     * @brief Disk queue policy. Default: DiskQueuePolicy::FifoDeleteOld
     * 
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

    /**
     * @brief Singleton instance of this class, allocated from instance() if needed
     */
    static EdgeEventQueueRK *_instance;
};

#endif // __EDGEEVENTQUEUERK_H

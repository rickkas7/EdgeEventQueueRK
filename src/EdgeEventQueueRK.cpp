#include "EdgeEventQueueRK.h"


static Logger _queueLogger("app.edgequeue");


EdgeEventQueueRK::EdgeEventQueueRK() {

}

EdgeEventQueueRK::~EdgeEventQueueRK() {
    
}

int EdgeEventQueueRK::setup() {
    int res = diskQueue.start(path, sizeLimit, policy);

    return res;
}

void EdgeEventQueueRK::loop() {
    if (!Particle.connected()) {
        return;
    }
    if (diskQueue.isEmpty()) {
        return;
    }

    if (nextCheck != 0 && System.millis() < nextCheck) {
        // Wait until checking again
        return;
    }

    const size_t storedDataMaxLen = particle::protocol::MAX_EVENT_NAME_LENGTH + particle::protocol::MAX_EVENT_DATA_LENGTH + 3;

    char storedData[storedDataMaxLen];
    size_t storedDataSize = storedDataMaxLen;

    if (!diskQueue.peekFront((uint8_t *)storedData, storedDataSize)) {
        return;
    }

    // Null terminate
    storedData[storedDataSize] = 0;

    char *endp;
    const char *eventName = strtok_r(storedData, "\n", &endp);
    const char *eventData = strtok_r(NULL, "\n", &endp);

    _queueLogger.trace("retrieved event from queue name=%s data=%s", eventName, eventData);

    // If the callback is not called within safetyCheckDelay (default: 60 seconds), try sending again.
    // If the send is really still in progress, CloudService send will do an immediately fail in the
    // BackgroundPublish and the delay will be advanced by immediateErrorDelay before trying again.
    nextCheck = System.millis() + safetyCheckDelay.count();

    int res = cloudServicePublish(eventName, eventData, publishFlags, priority, [this](CloudServiceStatus status) {
        if (status == CloudServiceStatus::SUCCESS) {
            // Successfully sent, remove from queue
            _queueLogger.trace("successfully sent, removing from queue");
            diskQueue.popFront();
            nextCheck = System.millis() + successDelay.count();
        }
        else {
            // Wait before retry on error. Default: 10 seconds.
            _queueLogger.trace("error sending, will retry after delay");
            nextCheck = System.millis() + sendErrorDelay.count();
        }
        return 0;
    });


    if (!res) {
        // Successfully started, will clear queue on success or failure
    } 
    else {
        // Immediate error occurred, wait and retry. Default: 5 seconds
        Log.trace("immediate error on send, will retry after delay");
        nextCheck = System.millis() + immediateErrorDelay.count();
    }

}


int EdgeEventQueueRK::publish(const char *eventName, const char *eventData) {

    if (!eventName) {
        return -1;
    }
    size_t eventNameLen = strlen(eventName);
    if (eventNameLen == 0 || eventNameLen > particle::protocol::MAX_EVENT_NAME_LENGTH) {
        return -1;
    }

    if (!eventData) {
        eventData = "";
    }

    size_t eventDataLen = strlen(eventData);
    if (eventDataLen > particle::protocol::MAX_EVENT_DATA_LENGTH) {
        return -1;
    }

    _queueLogger.trace("queueing event name=%s data=%s", eventName, eventData);

    const size_t storedDataMaxLen = particle::protocol::MAX_EVENT_NAME_LENGTH + particle::protocol::MAX_EVENT_DATA_LENGTH + 3;

    char storedData[storedDataMaxLen];
    snprintf(storedData, storedDataMaxLen, "%s\n%s\n", eventName, eventData);

    // DiskQueue takes a c-string (null terminated) but does not store the NULL on disk, so you need to restore it on read
    diskQueue.pushBack(storedData);

    return 0;
}


// [static] 
int EdgeEventQueueRK::cloudServicePublish(const char *eventName, const char *eventData, PublishFlags publishFlags, size_t priority, std::function<int(CloudServiceStatus)> cb) {
    int res = 0;

#if IS_TRACKER_EDGE
    // Monitor Edge
    res = CloudService::instance().send(
        eventData,
        publishFlags,
        CloudServicePublishFlags::NONE,
        [cb](CloudServiceStatus status, JSONValue *, const char *, const void *) { // cloud_service_send_cb_t
            _queueLogger.trace("publish complete status=%d", (int)status);
            if (cb) {
                return cb(status);
            }
            else {
                return 0;
            }
        },
        std::numeric_limits<system_tick_t>::max(), // No timeout, not used except with FULL_ACK anyway
        nullptr, // context
        eventName, 
        1, // req_id 
        priority); // level

#else
    // Monitor Edge
    res = CloudService::instance().send(
        eventData,
        publishFlags,
        CloudServicePublishFlags::NONE,
        [cb](CloudServiceStatus status, String &&) { // cloud_service_ack_callback
            _queueLogger.trace("publish complete status=%d", (int)status);
            if (cb) {
                return cb(status);
            }
            else {
                return 0;
            }
        },
        std::numeric_limits<system_tick_t>::max(), // No timeout, not used except with FULL_ACK anyway
        eventName, 
        1, // req_id 
        priority);
#endif

    return res;
}

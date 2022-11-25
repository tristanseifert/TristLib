#include <event2/event.h>

#include <cerrno>
#include <stdexcept>
#include <system_error>

#include "TristLib/Event.h"

using namespace TristLib::Event;

/**
 * @brief Create a new event source for a file descriptor
 *
 * @param loop Run loop to add the event source to
 * @param fd File descriptor to observe
 */
FileDescriptor::FileDescriptor(const std::shared_ptr<RunLoop> &loop, const int fd) : fd(fd) {
    // create the event
    auto ev = event_new(loop->getEvBase(), fd, EV_READ | EV_WRITE | EV_CLOSED | EV_PERSIST,
            [](auto fd, auto what, auto ctx) {
        reinterpret_cast<FileDescriptor *>(ctx)->handleEvents(what);
    }, this);
    if(!ev) {
        throw std::runtime_error("failed to create event");
    }

    this->event = ev;

    // then add it to the run loop
    auto err = event_add(ev, nullptr);
    if(err != 0) {
        throw std::runtime_error("event_add failed");
    }
}

/**
 * @brief Clean up the file descriptor event
 *
 * If requested during allocation, we will close the descriptor here as well.
 */
FileDescriptor::~FileDescriptor() {
    if(this->event) {
        event_free(this->event);
    }
}

/**
 * @brief Process events received from socket
 *
 * @param events Bit flags indicating what events are pending
 */
void FileDescriptor::handleEvents(const short events) {
    if(events & EV_READ) {
        if(this->readCallback && this->readEnabled) {
            (*this->readCallback)(this);
        }
    }
    if(events & EV_WRITE) {
        if(this->writeCallback && this->writeEnabled) {
            (*this->writeCallback)(this);
        }
    }
    if(events & EV_CLOSED) {
        if(this->eventCallback) {
            (*this->eventCallback)(this);
        }
    }
}

/**
 * @brief Enable reporting of events
 *
 * @param read When set, read events are enabled
 * @param write When set, write events are enabled
 */
void FileDescriptor::enableEvents(const bool read, const bool write) {
    if(read) {
        this->readEnabled = true;
    }
    if(write) {
        this->writeEnabled = true;
    }

}

/**
 * @brief Disable reporting of events
 *
 * @param read When set, read events are disabled
 * @param write When set, write events are disabled
 */
void FileDescriptor::disableEvents(const bool read, const bool write) {
    if(read) {
        this->readEnabled = false;
    }
    if(write) {
        this->writeEnabled = false;
    }
}

#include <event2/event.h>

#include "TristLib/Event.h"

using namespace TristLib::Event;

/**
 * @brief Initialize flag event
 *
 * This consists of an event that's not associated with any file descriptor: this means it can only
 * be triggered manually.
 *
 * @param loop Run loop to add the event source to
 */
Flag::Flag(const std::shared_ptr<RunLoop> &loop) {
    auto ev = event_new(loop->getEvBase(), -1, EV_READ, [](auto fd, auto events, auto ctx) {
        auto flag = reinterpret_cast<Flag *>(ctx);
        flag->callback(flag);
    }, this);

    this->event = ev;
}

/**
 * @brief Clean up flag resources
 */
Flag::~Flag() {
    if(this->event) {
        event_free(this->event);
    }
}

/**
 * @brief Trigger the event
 */
void Flag::signal() {
    event_active(this->event, EV_READ, 0);
}

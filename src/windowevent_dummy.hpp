#pragma once

#include "windowevent.hpp"

/**
 * @brief Returns nothing
 */
class WindowEventDummy : public WindowEvent
{
public:
    ~WindowEventDummy() override;

    std::optional<Event> popEvent() override;

protected:
    std::ostream &toStream(std::ostream &str) const override;
};

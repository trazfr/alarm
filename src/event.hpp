#pragma once

#include "toolbox_position.hpp"

enum class EventType
{
    Quit,  ///< action to quit requested (only SDL2 for now)
    Click, ///< click caught. Use Event::click to know the position
};

/**
 * @brief Click at the coordinates x,y
 */
struct EventClick
{
    float x; ///< x=0 is left of the screen, x=1 is right of the screen
    float y; ///< y=0 is top of the screen, y=1 is bottom of the screen
};

/**
 * @brief Maps an user event
 *
 * for now only click at a given position on screen or close the window on SDL2
 */
struct Event
{
    static Event createQuit();
    static Event createClick(float x, float y);

    EventType type;
    union {
        EventClick click; ///< only valid when type == EventType::Click
    };
};

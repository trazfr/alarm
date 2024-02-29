#pragma once

#include "toolbox_position.hpp"
#include "toolbox_time.hpp"

class Context;

/**
 * Convert a position (posX = 0..1, posY = 0..1) to the corresponding enum
 *
 * @arg posX=0 is left
 * @arg posX=1 is right
 * @arg posY=0 is up
 * @arg posY=1 is down
 */
constexpr Position getPositionFromCoordinates(float posX, float posY)
{
    constexpr float kSep1 = .25;
    constexpr float kSep2 = 1 - kSep1;

    return Position{(posX >= kSep1) + (posX > kSep2) + 3 * ((posY < kSep1) + (posY <= kSep2))};
}

/**
 * @brief Screen is a single display
 */
class Screen
{
public:
    explicit Screen(Context &ctx);
    virtual ~Screen() = 0;

    /**
     * Allocate all the data needed for the display.
     * This method is called when the Screen becomes active
     */
    virtual void enter() = 0;

    /**
     * Deallocate all the data needed for the display.
     * This method is called when the Screen becomes inactive
     */
    virtual void leave() = 0;

    /**
     * Called at each frame on the active screen
     */
    virtual void run(const Clock::time_point &time) = 0;

    /**
     * Callback in case of action (click at a given position on the screen)
     */
    virtual void handleClick(Position position) = 0;

protected:
    static constexpr int kMargin = 10;

    Context &ctx;
};

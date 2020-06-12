#pragma once

#include <memory>

class Config;

/**
 * @brief Render the analog clock's hands
 * 
 * The second hand is only displayed if enabled in the config
 */
class RendererClock
{
public:
    struct Impl;

    /**
     * Constructor
     * 
     * @param screenWidth used for the heigth / width ratio to have a round clock
     * @param screenHeight used for the heigth / width ratio to have a round clock
     * @param x position on screen of the center of the clock
     * @param y position on screen of the center of the clock
     * @param lengthHour length of the hour hand. Max is 1
     * @param widthHour width of the hour hand. Max is 1
     * @param lengthMin length of the minute hand. Max is 1
     * @param widthMin width of the minute hand. Max is 1
     * @param lengthSec length of the second hand. Max is 1. Only displayed if enabled in config
     * @param widthSec width of the second hand. Max is 1. Only displayed if enabled in config
     */
    RendererClock(const Config &config,
                  int screenWidth, int screenHeight,
                  int x, int y,
                  float lengthHour, float widthHour,
                  float lengthMin, float widthMin,
                  float lengthSec, float widthSec);
    ~RendererClock();

    /**
     * Display the clock hands (call OpenGL to perform the display)
     */
    void draw(int hour, int min, int sec, int millis);

private:
    std::unique_ptr<Impl> pimpl;
};

#pragma once

/**
 * @brief Simple position
 *
 * It is used in 2 cases:
 * @arg to detect clicks on screen
 * @arg to align the sprites
 */
enum class Position
{
    DownLeft = 0,
    Down = 1,
    DownRight = 2,

    Left = 3,
    Center = 4,
    Right = 5,

    UpLeft = 6,
    Up = 7,
    UpRight = 8,
};

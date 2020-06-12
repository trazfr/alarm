#include "screen.hpp"

Screen::Screen(Context &ctx)
    : ctx{ctx}
{
}

Screen::~Screen() = default;

static_assert(getPositionFromCoordinates(0, 0) == Position::UpLeft);
static_assert(getPositionFromCoordinates(.5, 0) == Position::Up);
static_assert(getPositionFromCoordinates(1, 0) == Position::UpRight);

static_assert(getPositionFromCoordinates(0, .5) == Position::Left);
static_assert(getPositionFromCoordinates(.5, .5) == Position::Center);
static_assert(getPositionFromCoordinates(1, .5) == Position::Right);

static_assert(getPositionFromCoordinates(0, 1) == Position::DownLeft);
static_assert(getPositionFromCoordinates(.5, 1) == Position::Down);
static_assert(getPositionFromCoordinates(1, 1) == Position::DownRight);

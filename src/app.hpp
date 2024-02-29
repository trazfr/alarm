#pragma once

#include <memory>

/**
 * @brief Represents the whole application
 */
class App
{
public:
    struct Impl;

    /**
     * @attention the class keeps a reference to constructor's arguments. Make sure they stay valid
     */
    explicit App(const char *configurationFile);
    ~App();

    /**
     * Runs the application
     *
     * This method only returns at exit time
     */
    void run();

private:
    std::unique_ptr<Impl> pimpl;
};

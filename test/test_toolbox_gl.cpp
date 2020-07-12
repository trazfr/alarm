// correct testing of OpenGL is very hardware dependent...

#include <gtest/gtest.h>

#include "toolbox_gl.hpp"
#include "window_factory.hpp"

class TestToolboxGl : public ::testing::Test
{
protected:
    // we need a valid OpenGL context
    void SetUp() override
    {
        factory.create(factory.getDriver(0), "dummy", 320, 240);
    }

    void TearDown() override
    {
        factory.clear();
    }

    WindowFactory factory;
};

TEST_F(TestToolboxGl, debugString)
{
    std::ostringstream str;
    glDebug(str);
}

TEST_F(TestToolboxGl, glCheckError)
{
    // no issue before
    glCheckError();

    // do something stupid
    glGetAttribLocation(-1, "BLA");
    try
    {
        glCheckError();
        FAIL() << "Should have thrown";
    }
    catch (Error &)
    {
    }
}

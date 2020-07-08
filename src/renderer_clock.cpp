#include "renderer_clock.hpp"

#include "config.hpp"
#include "gl_shader.hpp"
#include "gl_vbo.hpp"
#include "toolbox_gl.hpp"
#include "toolbox_io.hpp"

#include <array>
#include <cmath>
#include <vector>

namespace
{

constexpr std::array<GLfloat, 12> getVertices2D(GLfloat length, GLfloat width, GLfloat factor)
{
    // posW, posH, rotationFactor
    return {
        -width / 2, length, // Position 0
        factor,             //
        -width / 2, 0,      // Position 1
        factor,             //
        width / 2, 0,       // Position 2
        factor,             //
        width / 2, length,  // Position 3
        factor,             //
    };
}

constexpr std::array<GLushort, 6> getIndices(GLushort offset)
{
    return {
        offset, static_cast<GLushort>(offset + 1), static_cast<GLushort>(offset + 2), // 1
        offset, static_cast<GLushort>(offset + 2), static_cast<GLushort>(offset + 3), // 2
    };
}

constexpr GLfloat millisToUnit = 1. / 1000;
constexpr GLfloat secToUnit = 1;
constexpr GLfloat minToUnit = 60;
constexpr GLfloat hourToUnit = 3600;

} // namespace

struct RendererClock::Impl
{
    explicit Impl(GlProgram &&program,
                  GlVboArrayStatic &&vertices,
                  GlVboElementArray &&indices)
        : program{std::move(program)},
          vertices{std::move(vertices)},
          indices{std::move(indices)}
    {
    }

    GlProgram program;
    GlVboArrayStatic vertices;
    GlVboElementArray indices;
    GLint u_rotation = -1;
    GLint a_positionScreen = -1;
    GLint a_rotationFactor = -1;
};

RendererClock::RendererClock(const Config &config,
                             int screenWidth, int screenHeight,
                             int x, int y,
                             float lengthHour, float widthHour,
                             float lengthMin, float widthMin,
                             float lengthSec, float widthSec)
{
    // 1 every 12h
    const auto hourVertices = getVertices2D(lengthHour, widthHour, 1. / (12 * hourToUnit));
    const auto hourIndices = getIndices(0);
    // 1 every 1h
    const auto minVertices = getVertices2D(lengthMin, widthMin, 1. / hourToUnit);
    const auto minIndices = getIndices(hourIndices.back() + 1);
    // 1 every 1m
    const auto secVertices = getVertices2D(lengthSec, widthSec, 1. / minToUnit);
    const auto secIndices = getIndices(minIndices.back() + 1);

    std::vector<GLfloat> vertices;
    vertices.reserve(hourVertices.size() + minVertices.size() + secVertices.size());
    vertices.insert(vertices.end(), hourVertices.begin(), hourVertices.end());
    vertices.insert(vertices.end(), minVertices.begin(), minVertices.end());

    std::vector<GLushort> indices;
    indices.reserve(hourIndices.size() + minIndices.size() + secIndices.size());
    indices.insert(indices.end(), hourIndices.begin(), hourIndices.end());
    indices.insert(indices.end(), minIndices.begin(), minIndices.end());

    if (config.displaySeconds())
    {
        vertices.insert(vertices.end(), secVertices.begin(), secVertices.end());
        indices.insert(indices.end(), secIndices.begin(), secIndices.end());
    }

    pimpl = std::make_unique<Impl>(GlProgram{readFile(config.getShader("print_clock_hand.vert")), readFile(config.getShader("print_color.frag"))},
                                   GlVboArrayStatic{vertices.data(), vertices.size()},
                                   GlVboElementArray{indices.data(), indices.size()});

    pimpl->program.use();
    glUniform2f(pimpl->program.getUniformLocation("u_rotationAxis"), x * 2. / screenWidth - 1, y * 2. / screenHeight - 1);
    glUniform2f(pimpl->program.getUniformLocation("u_squareScreenFactor"),
                screenWidth / std::max<GLfloat>(screenWidth, screenHeight),
                screenHeight / std::max<GLfloat>(screenWidth, screenHeight));

    const auto &clockHandColor = config.getClockHandColor();
    glUniform3f(pimpl->program.getUniformLocation("u_color"), clockHandColor[0] / 255., clockHandColor[1] / 255., clockHandColor[2] / 255.);

    pimpl->u_rotation = pimpl->program.getUniformLocation("u_rotation");
    pimpl->a_positionScreen = pimpl->program.getAttribLocation("a_positionScreen");
    pimpl->a_rotationFactor = pimpl->program.getAttribLocation("a_rotationFactor");
}

RendererClock::~RendererClock() = default;

void RendererClock::draw(int hour, int min, int sec, int millis)
{
    if (hour >= 12)
    {
        hour -= 12;
    }
    const GLfloat rotation = hour * hourToUnit + min * minToUnit + sec * secToUnit + millis * millisToUnit;
    pimpl->program.use();

    glUniform1f(pimpl->u_rotation, rotation);
    pimpl->vertices.bind();
    pimpl->vertices.draw<GLfloat>(pimpl->a_positionScreen, 2, 0, 3);
    pimpl->vertices.draw<GLfloat>(pimpl->a_rotationFactor, 1, 2, 3);

    pimpl->indices.draw();
}

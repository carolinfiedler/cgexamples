#include "scene.h"

#include <iostream>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

Scene::Scene()
: m_drawMode(0)
, m_drawModeChanged(true)
, m_cameraMode(0)
, m_cameraModeChanged(true)
, m_rotate(false)
, m_angle(0.f)
, m_radius(10.f)
, m_nearPlane(1.f)
, m_farPlane(50.f)
, m_eye()
, m_direction()
, m_up(0.0f, 1.0f, 0.0f)
{
}

Scene::~Scene()
{
}

void Scene::initialize()
{
    m_startTimePoint = std::chrono::high_resolution_clock::now();

    // set color used when clearing the frame buffer
    gl::glClearColor(0.12f, 0.14f, 0.18f, 1.0f);

    example1.initialize();
    example2.initialize();
    model.initialize();
    model.setNumCubes(1);
}

void Scene::loadShaders()
{
    example1.loadShaders();
    example2.loadShaders();
}

void Scene::resize(int w, int h)
{
    m_width = w;
    m_height = h;
    m_median = m_width / 1000.f;
    
    // Define the area for the rasterizer that is used for the NDC mapping ([-1, 1]^2 x [0, 1])
    gl::glViewport(0, 0, m_width, m_height);
}

void Scene::changeDrawMode()
{
    m_drawMode = (++m_drawMode) % 3;
    m_drawModeChanged = true;
}

void Scene::changeCameraMode()
{
    m_cameraMode = (++m_cameraMode) % 2;
    m_cameraModeChanged = true;
}

void Scene::toggleRotation()
{
    m_rotate = !m_rotate;
}

void Scene::changeRadiusBy(float value)
{
    if(m_cameraMode == 1)
    {
        m_radius = std::min(m_farPlane, std::max(m_radius + value, 0.001f));
    }
}

void Scene::render(float speed)
{
    // clear offscreen-framebuffer color attachment (no depth attachment configured and thus omitting GL_DEPTH_BUFFER_BIT)
    gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
    
    // output
    if(m_cameraModeChanged){
        m_cameraModeChanged = false;
        static const auto cameraModes = std::array<std::string, 2>{
            "(I) centered: ",
            "(II) orbit: "};
        std::cout << "camera: " << cameraModes[m_cameraMode] << std::endl;
        m_startTimePoint = std::chrono::high_resolution_clock::now();
    }

    if (m_drawModeChanged)
    {
        m_drawModeChanged = false;
        static const auto drawModes = std::array<std::string, 3>{
                "(0) rendering screen aligned triangle: ",
                "(1) rendering with cubemap: ",
                "(2) both combined. left screen aligned triangle, right cubemap: "};
        std::cout << "    environment: " << drawModes[static_cast<int>(m_drawMode)] << std::endl;
        m_startTimePoint = std::chrono::high_resolution_clock::now();
    }

    //using msecs = std::chrono::milliseconds;
    //const auto now = std::chrono::high_resolution_clock::now();
    //auto time = static_cast<float>(std::chrono::duration_cast<msecs>(now - m_startTimePoint).count());
    //time *= 0.001f; // time is now in seconds

    // update angle
    if (m_rotate)
        m_angle += speed;
    if (m_angle >= 360.f) m_angle -= 360.f;
    else if (m_angle < 0.f) m_angle += 360.f;

    // update camera
    switch (m_cameraMode)
    {
    case 0:
        m_eye = glm::vec3(0.0f);
        m_direction = glm::vec3(sin(glm::radians(m_angle)), 0.f, cos(glm::radians(m_angle)));
        break;
    case 1:
        m_eye = glm::vec3(sin(glm::radians(m_angle + 180.f)) * m_radius, 0.0f, cos(glm::radians(m_angle + 180.f)) * m_radius); //add 180 degress to the angle to look in the same direction as in centered mode
        m_direction = -m_eye;
        break;
    }

    // create transformation
    const auto view = glm::lookAt(m_eye, m_direction, m_up);
    const auto projection = glm::perspective(glm::radians(80.f), static_cast<float>(m_width) / m_height, m_nearPlane, m_farPlane);
    const auto viewProjection = projection * view;

    // draw
    switch (m_drawMode)
    {
    case static_cast<int>(DrawMode::Skytriangle) :
        example1.render(viewProjection, m_eye);
        model.render(viewProjection);
        break;
    case static_cast<int>(DrawMode::Cubemap) :
        example2.render(viewProjection, m_eye);
        model.render(viewProjection);
        break;
    case static_cast<int>(DrawMode::Both) :
        gl::glScissor(0, 0, m_width / 2 - m_median, m_height);
        glEnable(gl::GLenum::GL_SCISSOR_TEST);
        example1.render(viewProjection, m_eye);
        model.render(viewProjection);

        gl::glScissor(m_width / 2 + m_median, 0, m_width / 2 - m_median, m_height);
        example2.render(viewProjection, m_eye);
        model.render(viewProjection);

        glDisable(gl::GLenum::GL_SCISSOR_TEST);
        break;
    }
}

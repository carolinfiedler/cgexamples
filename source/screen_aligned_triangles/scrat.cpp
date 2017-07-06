
#include "scrat.h"

#include <cmath>
#include <iostream>
#include <string>

#include <glbinding/gl32ext/gl.h>

#include <cgutils/common.h>


using namespace gl32core;


ScrAT::ScrAT()
: m_recorded(false)
, m_vaoMode(Mode::Two_Triangles_Two_DrawCalls)
, m_timeDurationMagnitude(3u)
{
}

ScrAT::~ScrAT()
{
    glDeleteBuffers(1, &m_VBO_screenAlignedTriangle);
    glDeleteBuffers(1, &m_VBO_screenAlignedQuad);

    glDeleteVertexArrays(1, &m_VAO_screenAlignedTriangle);
    glDeleteVertexArrays(1, &m_VAO_screenAlignedQuad);
    glDeleteVertexArrays(1, &m_VAO_empty);

    glDeleteProgram(m_program_record);
    glDeleteProgram(m_program_replay);
    glDeleteProgram(m_program_recordAVC);

    for (auto i = 0; i < m_vertexShaders.size(); ++i)
        glDeleteShader(m_vertexShaders[i]);
    for (auto i = 0; i < m_fragmentShaders.size(); ++i)
        glDeleteShader(m_fragmentShaders[i]);
    
    glDeleteFramebuffers(1, &m_fbo);

    glDeleteTextures(1, &m_texture);

    glDeleteBuffers(1, &m_acbuffer);
    glDeleteQueries(1, &m_query);
}

void ScrAT::initialize()
{
    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);

    glGenBuffers(1, &m_VBO_screenAlignedTriangle);
    glGenBuffers(1, &m_VBO_screenAlignedQuad);

    static const float verticesScrAT[] = { -1.f, -3.f, -1.f, 1.f, 3.f, 1.f };
    static const float verticesScrAQ[] = { -1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f };

    glGenVertexArrays(1, &m_VAO_screenAlignedTriangle);
    glGenVertexArrays(1, &m_VAO_screenAlignedQuad);
    glGenVertexArrays(1, &m_VAO_empty);

    glBindVertexArray(m_VAO_screenAlignedTriangle);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO_screenAlignedTriangle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sizeof(verticesScrAT), verticesScrAT, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
    glBindVertexArray(0);

    glBindVertexArray(m_VAO_screenAlignedQuad);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO_screenAlignedQuad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sizeof(verticesScrAQ), verticesScrAQ, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
    glBindVertexArray(0);

    glBindVertexArray(m_VAO_empty);
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // create resources for record program

    m_program_record = glCreateProgram();
    m_vertexShaders[0] = glCreateShader(GL_VERTEX_SHADER);
    m_fragmentShaders[0] = glCreateShader(GL_FRAGMENT_SHADER);

    glAttachShader(m_program_record, m_vertexShaders[0]);
    glAttachShader(m_program_record, m_fragmentShaders[0]);

    glBindFragDataLocation(m_program_record, 0, "out_color");

    //create resources for replay program

    m_program_replay = glCreateProgram();
    m_vertexShaders[1] = glCreateShader(GL_VERTEX_SHADER);
    m_fragmentShaders[1] = glCreateShader(GL_FRAGMENT_SHADER);

    glAttachShader(m_program_replay, m_vertexShaders[1]);
    glAttachShader(m_program_replay, m_fragmentShaders[1]);

    glBindFragDataLocation(m_program_replay, 0, "out_color");
    
    //create resources for record AVC program

    m_program_recordAVC = glCreateProgram();
    m_vertexShaders[2] = glCreateShader(GL_VERTEX_SHADER);
    m_geometryShaders[0] = glCreateShader(GL_GEOMETRY_SHADER);

    glAttachShader(m_program_recordAVC, m_vertexShaders[2]);
    glAttachShader(m_program_recordAVC, m_geometryShaders[0]);
    glAttachShader(m_program_recordAVC, m_fragmentShaders[0]);

    glBindFragDataLocation(m_program_recordAVC, 0, "out_color");

    loadShaders();

    // Fragment Index Render Target

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_R32F), m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));

    // create and configure framebuffer

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);

    static const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // create and configure atomic counter

    glGenBuffers(1, &m_acbuffer);
    glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, m_acbuffer);
    glBufferData(gl32ext::GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0);

    // setup time measurement

    glGenQueries(1, &m_query);
}

bool ScrAT::loadShader(const std::string & sourceFile, const gl::GLuint & shader)
{
    const auto shaderSource = cgutils::textFromFile(sourceFile.c_str());
    const auto shaderSource_ptr = shaderSource.c_str();
    if (shaderSource_ptr)
        glShaderSource(shader, 1, &shaderSource_ptr, 0);

    glCompileShader(shader);
    return cgutils::checkForCompilationError(shader, sourceFile);
}

bool ScrAT::loadShaders()
{
    static const auto sourceFiles = std::array<std::string, 6>{
        "data/screen_aligned_triangles/record.vert",
        "data/screen_aligned_triangles/record-empty.vert",
        "data/screen_aligned_triangles/record.geom",
        "data/screen_aligned_triangles/record.frag",
        "data/screen_aligned_triangles/replay.vert",
        "data/screen_aligned_triangles/replay.frag"  };

    // setup shaders of render program 

    bool success = loadShader(sourceFiles[0], m_vertexShaders[0]);
    success &= loadShader(sourceFiles[3], m_fragmentShaders[0]);
    if (!success)
        return false;

    gl::glLinkProgram(m_program_record);

    success &= cgutils::checkForLinkerError(m_program_record, "record program");
    if (!success)
        return false;

    // setup shaders of replay program

    success = loadShader(sourceFiles[4], m_vertexShaders[1]);
    success &= loadShader(sourceFiles[5], m_fragmentShaders[1]);
    if (!success)
        return false;

    gl::glLinkProgram(m_program_replay);

    success &= cgutils::checkForLinkerError(m_program_replay, "replay program");
    if (!success)
        return false;
 
    // setup shaders of record AVC program

    success = loadShader(sourceFiles[1], m_vertexShaders[2]);
    success &= loadShader(sourceFiles[2], m_geometryShaders[0]);
    if (!success)
        return false;

    gl::glLinkProgram(m_program_recordAVC);

    success &= cgutils::checkForLinkerError(m_program_recordAVC, "record AVC program");
    if (!success)
        return false;

    loadUniformLocations();

    return true;
}

void ScrAT::loadUniformLocations()
{
    glUseProgram(m_program_recordAVC);
    m_uniformLocation_benchmark = glGetUniformLocation(m_program_record, "benchmark");

    glUseProgram(m_program_replay);
    m_uniformLocation_indexSampler = glGetUniformLocation(m_program_replay, "fragmentIndex");
    m_uniformLocation_indexThreshold = glGetUniformLocation(m_program_replay, "threshold");

    glUseProgram(m_program_recordAVC);
    m_uniformLocation_benchmark_avc = glGetUniformLocation(m_program_recordAVC, "benchmark");

    glUseProgram(0);
}

void ScrAT::resize(int w, int h)
{
    m_width = w;
    m_height = h;

    m_recorded = false;

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_R32F), m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ScrAT::record()
{
    std::cout << "benchmarking ... ";

    auto elapsed = std::uint64_t{ 0 };
    for (auto i = 0; i < 1000; ++i)
        elapsed += record(true); // benchmark

    static const auto modes = std::array<std::string, 5>{
        "(0) two triangles, two draw calls :         ",
            "(1) two triangles, single draw call (quad): ",
            "(2) single triangle, single draw call :     ",
            "(3) fill rectangle ext, single draw call:   ",
            "(4) AVC, single draw call:                  " };
    std::cout << modes[unsigned int(m_vaoMode)] << cgutils::humanTimeDuration(elapsed / 1000) << std::endl;

    record(false);
    m_recorded = true;

    m_lastIndex = 0;
    m_time = std::chrono::high_resolution_clock::now();
}

std::uint64_t ScrAT::record(const bool benchmark)
{
    glViewport(0, 0, m_width, m_height);

    // clear record buffer

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClear(GL_COLOR_BUFFER_BIT);

    static const GLfloat color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 0, color);

    // reset atomic counter

    glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, m_acbuffer);

    const auto counter = 0u;
    glBufferSubData(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &counter);
    glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0);

    glBindBufferBase(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0, m_acbuffer);
    
    // draw

    auto elapsed = std::uint64_t{ 0 };

    if (m_vaoMode == Mode::AVC_One_DrawCall)
    {
        glUseProgram(m_program_recordAVC);
        glUniform1i(m_uniformLocation_benchmark_avc, static_cast<GLint>(benchmark));
    } else {
        glUseProgram(m_program_record);
        glUniform1i(m_uniformLocation_benchmark, static_cast<GLint>(benchmark));
    }

    if(benchmark)
        glBeginQuery(gl::GL_TIME_ELAPSED, m_query);

    switch(m_vaoMode)
    {
    case Mode::Two_Triangles_Two_DrawCalls:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(m_VAO_screenAlignedQuad);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawArrays(GL_TRIANGLES, 1, 3);
        break;
    case Mode::Two_Triangles_One_DrawCall:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(m_VAO_screenAlignedQuad);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        break;
    case Mode::One_Triangle_One_DrawCall:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(m_VAO_screenAlignedTriangle);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        break;
    case Mode::Quad_Fill_Rectangle:
        glPolygonMode(GL_FRONT_AND_BACK, gl::GL_FILL_RECTANGLE_NV);
        glBindVertexArray(m_VAO_screenAlignedQuad);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        break;
    case Mode::AVC_One_DrawCall:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(m_VAO_empty);
        glDrawArrays(GL_POINTS, 0, 1);
        break;
    }

    if (benchmark)
    {
        glEndQuery(gl::GL_TIME_ELAPSED);       

        auto done = 0;
        while (!done)
            glGetQueryObjectiv(m_query, GL_QUERY_RESULT_AVAILABLE, &done);

        glGetQueryObjectui64v(m_query, GL_QUERY_RESULT, &elapsed);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBufferBase(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0, 0);  

    if (!benchmark)
    {
        glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, m_acbuffer);

        auto data = 0u;
        gl32ext::glMemoryBarrier(gl32ext::GL_ATOMIC_COUNTER_BARRIER_BIT);
        glGetBufferSubData(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &data);

        glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0);
    }

    return elapsed;
}

void ScrAT::replay()
{
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glUseProgram(m_program_replay);
    glUniform1f(m_uniformLocation_indexThreshold, m_currentIndex);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(m_VAO_screenAlignedTriangle);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glUseProgram(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ScrAT::render()
{
    if (!m_recorded)
        record();

    replay();
    updateThreshold();
}

void ScrAT::updateThreshold()
{
    m_currentIndex = m_lastIndex + 0.1f * powf(10.f, static_cast<float>(m_timeDurationMagnitude))
        * msecs(std::chrono::high_resolution_clock::now() - m_time).count();
}

void ScrAT::resetAC()
{
    m_recorded = false;
}

void ScrAT::switchVAO()
{
    m_recorded = false;
    m_vaoMode = Mode((static_cast<unsigned int>(m_vaoMode) + 1u) % 5u);
}

void ScrAT::incrementReplaySpeed()
{
    updateThreshold();
    
    m_lastIndex = m_currentIndex;
    m_time = std::chrono::high_resolution_clock::now();

    ++m_timeDurationMagnitude;
}

void ScrAT::decrementReplaySpeed()
{
    if (m_timeDurationMagnitude == 0)
        return;

    updateThreshold();

    m_lastIndex = m_currentIndex;
    m_time = std::chrono::high_resolution_clock::now();

    --m_timeDurationMagnitude;
}

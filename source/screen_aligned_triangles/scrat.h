
#include <glbinding/gl32core/gl.h>  // this is a OpenGL feature include; it declares all OpenGL 3.2 Core symbols

#include <chrono>


// For more information on how to write C++ please adhere to: 
// http://cginternals.github.io/guidelines/cpp/index.html

class ScrAT
{
public:
    enum class Mode : unsigned int {
        Two_Triangles_Two_DrawCalls = 0u,
        Two_Triangles_One_DrawCall = 1u,
        One_Triangle_One_DrawCall = 2u,
        Quad_Fill_Rectangle = 3u,
        AVC_One_DrawCall = 4u
    };

    static const std::array<std::string, 5> s_modeDescriptions;

public:
    ScrAT();
    ~ScrAT();

    void initialize();
    bool loadShaders();
    void resize(int w, int h);

    void render();
    void benchmark();
    void reset();

    void switchDrawMode();
    void switchDrawMode(const Mode mode);

    void incrementReplaySpeed();
    void decrementReplaySpeed();

protected:
    void loadUniformLocations();

    std::uint64_t record(bool benchmark);
    void replay();

    void updateThreshold();

protected:
    gl::GLuint m_fbo;
    gl::GLuint m_texture;
    int m_width;
    int m_height;

    Mode m_vaoMode;

    gl::GLuint m_VAO_screenAlignedTriangle,
               m_VAO_screenAlignedQuad,
               m_VAO_empty;

    gl::GLuint m_VBO_screenAlignedTriangle,
               m_VBO_screenAlignedQuad;

    gl::GLuint m_program_record,
               m_program_replay,
               m_program_recordAVC;

    gl::GLuint m_vertexShader_record,
               m_vertexShader_replay,
               m_vertexShader_recordAVC;

    gl::GLuint m_geometryShader_recordAVC;
    
    gl::GLuint m_fragmentShader_record,
               m_fragmentShader_replay;

    gl::GLuint m_uniformLocation_indexSampler,
               m_uniformLocation_indexThreshold,
               m_uniformLocation_benchmark,
               m_uniformLocation_benchmark_avc;

    gl::GLuint m_acbuffer;   
    
    bool m_recorded;
    float m_lastIndex;
    float m_currentIndex;

    gl::GLuint m_query;

    using msecs = std::chrono::duration<float, std::chrono::milliseconds::period>;

    std::chrono::time_point<std::chrono::high_resolution_clock> m_time;
    std::uint32_t m_timeDurationMagnitude;
};

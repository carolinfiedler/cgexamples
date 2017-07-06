
#include <glbinding/gl32core/gl.h>  // this is a OpenGL feature include; it declares all OpenGL 3.2 Core symbols

#include <chrono>


// For more information on how to write C++ please adhere to: 
// http://cginternals.github.io/guidelines/cpp/index.html

class ScrAT
{
public:
    ScrAT();
    ~ScrAT();

    void initialize();
	static bool loadShader(const std::string & sourceFile, const gl::GLuint & shader);
    bool loadShaders();

    void resize(int w, int h);
    void render();

    void resetAC();
    void switchVAO();

    void incrementReplaySpeed();
    void decrementReplaySpeed();

protected:
	enum class Mode : unsigned int {
		Two_Triangles_Two_DrawCalls = 0u,
		Two_Triangles_One_DrawCall = 1u,
		One_Triangle_One_DrawCall = 2u,
		Quad_Fill_Rectangle = 3u,
		AVC_One_DrawCall = 4u
	};

protected:
    void loadUniformLocations();

    void record();
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

    std::array<gl::GLuint, 3> m_vertexShaders;
    std::array<gl::GLuint, 2> m_geometryShaders;
    std::array<gl::GLuint, 2> m_fragmentShaders;

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

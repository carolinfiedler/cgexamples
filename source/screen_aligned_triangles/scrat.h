
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
	static bool loadShader(const std::string & sourceFile, const GLuint & shader);
    bool loadShaders();

    void resize(int w, int h);
    void render();
    void execute();

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

	enum Program {
		Record = 0,
		Replay = 1, 
		RecordAVC = 2
	};

	enum VAO {
		ScreenAlignedTriangle = 0,
		ScreenAlignedQuad = 1,
		Empty = 2
	};

protected:
    void loadUniformLocations();

    std::uint64_t record(bool benchmark);
    void replay();

    void updateThreshold();

protected:
    std::array<gl::GLuint, 2> m_vbos;

    std::array<gl::GLuint, 3> m_programs;
    std::array<gl::GLuint, 3> m_vertexShaders;
    std::array<gl::GLuint, 2> m_geometryShaders;
    std::array<gl::GLuint, 2> m_fragmentShaders;

    std::array<gl::GLuint, 3> m_vaos;
    gl::GLuint m_fbo;
    std::array<gl::GLuint, 1> m_textures;
    std::array<gl::GLuint, 4> m_uniformLocations;

    gl::GLuint m_query;
    gl::GLuint m_acbuffer;   

    bool m_recorded;
    std::array<float, 3> m_threshold; // { last, current, max }
	Mode m_vaoMode;

    using msecs = std::chrono::duration<float, std::chrono::milliseconds::period>;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_time;
    std::uint32_t m_timeDurationMagnitude;

    int m_width;
    int m_height;
};

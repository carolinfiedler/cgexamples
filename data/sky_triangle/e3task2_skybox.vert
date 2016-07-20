#version 150 core

in vec3 in_vertex;

uniform mat4 transform;

out vec3 v_uv;

void main()
{
	vec4 vertex = transform * vec4(in_vertex * 1.0, 1.0);
    gl_Position = vertex.xyww;
    v_uv = in_vertex;
}

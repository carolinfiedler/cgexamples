#version 330

uniform vec2 scale; 

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in vec4 v_color[];

out vec2 g_uv;
out vec4 g_color;

void main()
{
	vec4 p = gl_in[0].gl_Position;

	vec4 u = vec4(scale.x, 0.0, 0.0, 0.0);	
	vec4 v = vec4(0.0, scale.y, 0.0, 0.0);

	// frustum culling 
	vec2 c = clamp(abs(p.xy) / p.w, 0.0, 1.0);
	if(any(equal(c, vec2(1.0))))
		return;

	g_color = v_color[0];

	g_uv = vec2(-1.0, -1.0);
	gl_Position = p + u + v;
	EmitVertex();

	g_uv = vec2(-1.0,  1.0);
	gl_Position = p + u - v;
	EmitVertex();
	
	g_uv = vec2( 1.0, -1.0);
	gl_Position = p - u + v;
	EmitVertex();

	g_uv = vec2( 1.0,  1.0);
	gl_Position = p - u - v;
	EmitVertex();

	EndPrimitive();	
}
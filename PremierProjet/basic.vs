attribute vec2 a_position;
attribute vec3 a_color;
varying vec4 v_color;

uniform mat4  u_rotationZ;
uniform int u_time;

void main(void) 
{
	vec4 pos = u_rotationZ * vec4(a_position, 0.0, 1.0);
	gl_Position = pos;
	v_color = vec4(a_color, 1.0);
}

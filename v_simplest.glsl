#version 330 core

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

in vec4 vertex;
in vec4 normal;
in vec4 color;

out vec4 ic;
out vec4 l;
out vec4 n;
out vec4 v;

uniform vec4 light_pos;

void main(void) {
	
	l = normalize(V * light_pos - V * M * vertex);
	n = normalize(V * M * normal);
	v = normalize(vec4(0,0,0,1) - V * M * vertex);

	gl_Position = P * V * M * vertex;
	ic = color;

}
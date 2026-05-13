#version 330 core

out vec4 pixelColor;

in vec4 ic;
in vec4 l;
in vec4 n;
in vec4 v;

void main(void) {
	
	vec4 ml = normalize(l);
	vec4 mn = normalize(n);
	vec4 mv = normalize(v);

	vec4 r = reflect(-ml, mn);
	float nl = clamp(dot(mn, ml), 0.0, 1.0);
	float rv = clamp(dot(r, mv), 0.0, 1.0);
	rv = pow(rv, 25.0);

	pixelColor = ic * nl + rv;
}
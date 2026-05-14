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

	float nl = clamp(dot(mn, ml), 0.0, 1.0);

	//vec4 r = reflect(-ml, mn);
	//float rv = clamp(dot(r, mv), 0.0, 1.0);
	//rv = pow(rv, 25.0);
	//pixelColor = ic * nl + rv;

	vec4 halwayDir = normalize(ml + mv); // Blinn-Phong wyciąga "średnią" z kamery i światła
	float spec = clamp(dot(mn, halwayDir), 0.0, 1.0); // Sprawdza kąt między tą średnią a normalną
	spec = pow(spec, 64.0);
	pixelColor = ic * (nl+0.2) + spec;
}
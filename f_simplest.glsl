#version 330 core

out vec4 pixelColor;

in vec4 ic;
in vec4 l;
in vec4 l2;
in vec4 n;
in vec4 v;

void main(void) {
	
	vec4 ml = normalize(l);
	vec4 ml2 = normalize(l2);
	vec4 mn = normalize(n);
	vec4 mv = normalize(v);

	vec4 cold_color = vec4(0.3, 0.5, 1.0, 1.0); // zimne swiatło 
	vec4 warm_color = vec4(1.0, 0.9, 0.75, 1.0); // ciepłe światło

	//float nl = clamp(dot(mn, ml), 0.0, 1.0);

	//vec4 r = reflect(-ml, mn);
	//float rv = clamp(dot(r, mv), 0.0, 1.0);
	//rv = pow(rv, 25.0);
	//pixelColor = ic * nl + rv;

	// światło 1 ciepłe
	vec4 half_vec1 = normalize(ml + mv); // Blinn-Phong wyciąga "średnią" z kamery i światła
	float nl1 = max(dot(mn, ml), 0.0); // Rozproszenie
	float hn1 = max(dot(mn, half_vec1), 0.0); // Odbicie
	float specular1 = pow(hn1, 64.0); // Odbicie lustrzane

	// światło 2 zimne
	vec4 half_vec2 = normalize(ml2 + mv);
	float nl2 = max(dot(mn, ml2), 0.0);
	float hn2 = max(dot(mn, half_vec2), 0.0);
	float specular2 = pow(hn2, 16.0); // Mniejsza potęga = szerszy, łagodniejszy blask z tyłu

	vec4 ambient = ic * 0.15; // Światło otoczenia

	// Sumujemy kolory bazowe (diffuse) dla obu świateł
	vec4 diffuse = (ic * nl1 * warm_color) + (ic * nl2 * cold_color * 0.6); // Zimne światło jest celowo trochę słabsze (* 0.6)

	// Sumujemy odblaski (specular) dla obu świateł
	vec4 spec = (vec4(1.0) * specular1 * warm_color) + (vec4(1.0) * specular2 * cold_color * 0.6);

	pixelColor = clamp(ambient + diffuse + spec, 0.0, 1.0);
}
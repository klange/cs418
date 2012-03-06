varying vec3 vertex_light_position;
varying vec3 vertex_normal;

void main() {
	vec4 color;
	color.r = 0.2;
	color.g = 0.8;
	color.b = 0.2;
	color.a = 0.8;
	vec4 env;
	env.r   = 0.2;
	env.g   = 0.4;
	env.b   = 0.6;
	float diffuse_value = max(dot(vertex_normal, vertex_light_position), 0.1);
	gl_FragColor = color * diffuse_value + env * 0.5;
	gl_FragColor.a = color.a;
}

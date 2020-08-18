#version 330 core
const vec3 light_pos = vec3(1000.0,1000.0,1000.0);

in vec3 frag_pos;
in vec3 normal;
in vec3 oocolor;
out vec4 out_color;

void main() {
   vec3 norm = normalize(normal);
   vec3 light_dir = normalize(light_pos - frag_pos);
   float diff = min(max(dot(norm, light_dir), 0.f), .6f);
   out_color = vec4((.4f + diff) * oocolor, 1.0f);
}


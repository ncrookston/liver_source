
#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 28) out;

in float oradius[];
flat out vec4 oocolor;

uniform mat4 view;
uniform mat4 proj;

const vec3 light_pos = -vec3(1000.0,-1000.0,1000.0);
const vec3 y = vec3(.0006,0,0);
const vec3 x = vec3(0,.000693,0);
const vec3 z = vec3(0,0,.0015);
const vec3 color = vec3(7/255.f, 32/255.f, 73/255.f);
vec3 h(vec4 v) { return vec3(v) / v.w; }
vec4 project(vec3 v) { return proj * view * vec4(v,1); }
vec4 calc_color(vec3 v1, vec3 v2, vec3 v3) {
  vec3 norm = normalize(cross(v2-v1, v3-v1));
  vec3 light_dir = normalize(light_pos - v1);
  float diff = min(max(dot(norm, light_dir), 0.f), .6f);
  return vec4((.4f + diff) * color, 1.0f);
}

void main() {
  float scale = 900 * oradius[0];
  vec3 ctr = h(gl_in[0].gl_Position);
  vec3 u1 = ctr + (-x - y/2 - z/2) * scale;
  vec3 u2 = ctr + (-x + y/2 - z/2) * scale;
  vec3 u3 = ctr + (   - y   - z/2) * scale;
  vec3 u4 = ctr + (   + y   - z/2) * scale;
  vec3 u5 = ctr + (+x - y/2 - z/2) * scale;
  vec3 u6 = ctr + (+x + y/2 - z/2) * scale;
  oocolor = calc_color(u1, u2, u3);
  gl_Position = project(u1); EmitVertex();
  gl_Position = project(u2); EmitVertex();
  gl_Position = project(u3); EmitVertex();
  gl_Position = project(u4); EmitVertex();
  gl_Position = project(u5); EmitVertex();
  gl_Position = project(u6); EmitVertex();
  EndPrimitive();
  oocolor = calc_color(u6, u5, u6+z);
  gl_Position = project(u6); EmitVertex();
  gl_Position = project(u6 + z * scale); EmitVertex();
  gl_Position = project(u5); EmitVertex();
  gl_Position = project(u5 + z * scale); EmitVertex();
  oocolor = calc_color(u3, u3+z, u5+z);
  gl_Position = project(u3); EmitVertex();
  gl_Position = project(u3 + z * scale); EmitVertex();
  oocolor = calc_color(u1, u1+z, u3+z);
  gl_Position = project(u1); EmitVertex();
  gl_Position = project(u1 + z * scale); EmitVertex();
  oocolor = calc_color(u2, u2+z, u1+z);
  gl_Position = project(u2); EmitVertex();
  gl_Position = project(u2 + z * scale); EmitVertex();
  oocolor = calc_color(u4, u4+z, u2+z);
  gl_Position = project(u4); EmitVertex();
  gl_Position = project(u4 + z * scale); EmitVertex();
  oocolor = calc_color(u6, u6+z, u4+z);
  gl_Position = project(u6); EmitVertex();
  gl_Position = project(u6 + z * scale); EmitVertex();
  EndPrimitive();

  oocolor = calc_color(u1+z, u3+z, u2+z);
  gl_Position = project(u1 + z * scale); EmitVertex();
  gl_Position = project(u2 + z * scale); EmitVertex();
  gl_Position = project(u3 + z * scale); EmitVertex();
  gl_Position = project(u4 + z * scale); EmitVertex();
  gl_Position = project(u5 + z * scale); EmitVertex();
  gl_Position = project(u6 + z * scale); EmitVertex();
  EndPrimitive();
}



#version 330 core
#define PER_VERTEX 8
layout(triangles) in;
layout(triangle_strip, max_vertices = 48) out;

in float oradius[];
in vec3 ocolor[];

out vec3 frag_pos;
out vec3 normal;
out vec3 oocolor;
uniform mat4 view;
uniform mat4 proj;

vec3 h(vec4 v) { return vec3(v) / v.w; }
vec4 project(vec3 v) { return proj * view * vec4(v,1); }
void emit_verts(vec3 p0, vec3 p1, vec3 v00, vec3 v10, vec3 v01, vec3 v11, float theta) {
  vec3 normal0 = v00 * cos(theta) + v10 * sin(theta);
  vec3 normal1 = v01 * cos(theta) + v11 * sin(theta);

  normal = normal1;
  frag_pos = p0;
  gl_Position = project(p0 + normal0);
  EmitVertex();
  frag_pos = p1;
  gl_Position = project(p1 + normal1);
  EmitVertex();
}
void main() {
  vec3 p0 = h(gl_in[0].gl_Position);
  vec3 p1 = h(gl_in[1].gl_Position);
  vec3 p2 = h(gl_in[2].gl_Position);
  vec3 v00 = oradius[1] * normalize(cross(p1 - p0, vec3(1,0,0)));
  vec3 v10 = oradius[1] * normalize(cross(v00, p1 - p0));
  //vec3 v00 = oradius[1] * normalize(cross(p2 - p1, vec3(1,0,0)));
  //vec3 v10 = oradius[1] * normalize(cross(v00, p2 - p1));
  vec3 v01 = oradius[2] * normalize(cross(p2 - p1, vec3(1,0,0)));
  vec3 v11 = oradius[2] * normalize(cross(v01, p2 - p1));
  oocolor = ocolor[0];
  for (float theta = 0; theta <= PER_VERTEX; theta += 1) {
    if (oradius[1] > 2 * oradius[2])
      emit_verts(p1, p2, v01, v11, v01, v11, 2 * 3.14159265358979323 * theta / PER_VERTEX);
    else
      emit_verts(p1, p2, v00, v10, v01, v11, 2 * 3.14159265358979323 * theta / PER_VERTEX);
  }

  EndPrimitive();
}


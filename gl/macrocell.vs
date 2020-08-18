#version 330 core

in vec3 loc;
in float radius;
out float oradius;

void main() {
  gl_Position = vec4(loc, 1.0);
  oradius = radius;
}


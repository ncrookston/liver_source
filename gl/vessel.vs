#version 330 core

const vec3 light_brown = vec3(252/255.f, 235/255.f, 182/255.f);
const vec3 dark_brown = vec3(94/255.f,  65/255.f,  47/255.f);
const vec3 orange = vec3(232/255.f,  117/255.f,  22/255.f);
const vec3 yellow = vec3(231/255.f,  164/255.f,  50/255.f);
const vec3 red = vec3(.6f, 0.f, 0.f);
in vec3 loc;
in float radius;
in float idx;
out float oradius;
out vec3 ocolor;

void main() {
   gl_Position = vec4(loc, 1.0);
   oradius = radius;
#if 0
   if (idx < 1)
     ocolor = light_brown;
   else if (idx < 3)
     ocolor = orange;
   else if (idx < 5)
     ocolor = yellow;
   else
     ocolor = dark_brown;
#else
   ocolor = red;
#endif
}


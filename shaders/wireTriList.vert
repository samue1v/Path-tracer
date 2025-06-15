#version 450

layout(binding = 0) uniform MV {
    mat4 view;
    mat4 proj;
} mv;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = mv.proj*mv.view*vec4(inPosition.x,inPosition.y,inPosition.z,1.f);
    fragColor = inColor;
}


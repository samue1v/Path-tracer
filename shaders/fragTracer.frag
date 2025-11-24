#version 450

layout(location = 0) in vec2 vUV;

layout(location = 0) out vec4 outColor;

// Combined image sampler passed through your descriptor set layout
layout(set = 0, binding = 7) uniform sampler2D uTexture;

void main() {
    outColor = texture(uTexture, vUV);
    
    //outColor = vec4(0.,1.,0.,1.);

}

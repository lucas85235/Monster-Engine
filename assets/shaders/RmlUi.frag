#version 330 core

in vec4 v_Color;
in vec2 v_TexCoord;

out vec4 color;

uniform sampler2D u_Texture;
uniform int u_UseTexture; // 1 if textured, 0 if not

void main()
{
    if (u_UseTexture == 1)
        color = texture(u_Texture, v_TexCoord) * v_Color;
    else
        color = v_Color;
}

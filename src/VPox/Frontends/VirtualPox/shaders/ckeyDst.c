/* $Id: ckeyDst.c $ */
#extension GL_ARB_texture_rectangle : enable
uniform sampler2DRect uDstTex;
uniform vec4 uDstClr;
void vpoxCKeyDst(void)
{
    vec4 dstClr = texture2DRect(uDstTex, vec2(gl_TexCoord[1]));
    vec3 difClr = dstClr.rgb - uDstClr.rgb;
    if(any(greaterThan(difClr, vec3(0.01, 0.01, 0.01)))
        || any(lessThan(difClr, vec3(-0.01, -0.01, -0.01))))
            discard;
}

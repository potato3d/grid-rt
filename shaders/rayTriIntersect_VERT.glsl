#version 110

varying vec3 v_rayDir;
varying vec3 v_v0;
varying vec3 v_v1;
varying vec3 v_v2;

void main( void )
{
    gl_Position = ftransform();

 	// Ray direction to be interpolated by rasterizer
    v_rayDir = gl_MultiTexCoord0.xyz;

	// Triangle vertices
    v_v0 = gl_MultiTexCoord1.xyz;
    v_v1 = gl_MultiTexCoord2.xyz;
    v_v2 = gl_MultiTexCoord3.xyz;
}

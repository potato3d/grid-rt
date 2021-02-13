#version 110

varying vec3 v_rayDir;
varying vec2 v_cellInfo;

void main( void )
{
 	// Ray direction to be interpolated by rasterizer
    v_rayDir = gl_MultiTexCoord0.xyz;

	// Regular pipeline
    gl_Position = ftransform();
}

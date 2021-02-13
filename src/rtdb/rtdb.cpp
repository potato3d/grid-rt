#include <rtdb/rtdb.h>
#include <rt/Context.h>

#include "Teapot.h"
#include "Cube.h"

using namespace rtdb;

void rtdb::loadTriangle()
{
	rt::Context* ctx = rt::Context::current();
	ctx->beginPrimitive( RT_TRIANGLES );
	ctx->setNormal( 0, 0, 1 );
	ctx->addVertex( 0, 0, 0 );
	ctx->addVertex( 1, 0, 0 );
	ctx->addVertex( 0, 1, 0 );
	ctx->endPrimitive();
}

void rtdb::loadLogo()
{
	rt::Context* ctx = rt::Context::current();

	ctx->beginPrimitive( RT_TRIANGLES );

	ctx->setNormal( 0.0f, 0.0f, 1.0f );

	// Triangle 1
	ctx->setColor( 0.0f, 0.0f, 1.0f );
	ctx->addVertex( 0.0f, 0.0f, 0.0f );

	ctx->setColor( 1.0f, 0.0f, 0.0f );
	ctx->addVertex( 1.0f, 0.0f, 0.0f );

	ctx->setColor( 0.0f, 1.0f, 0.0f );
	ctx->addVertex( 0.0f, 1.0f, 0.0f );

	// Triangle 2
	ctx->setColor( 1.0f, 0.0f, 0.0f );
	ctx->addVertex( 2.0f, 0.0f, 0.0f );

	ctx->setColor( 0.0f, 1.0f, 0.0f );
	ctx->addVertex( 3.0f, 0.0f, 0.0f );

	ctx->setColor( 0.0f, 0.0f, 1.0f );
	ctx->addVertex( 3.0f, 1.0f, 0.0f );

	// Triangle 3
	ctx->setColor( 1.0f, 0.0f, 0.0f );
	ctx->addVertex( 1.5f, 0.0f, 0.0f );

	ctx->setColor( 0.0f, 1.0f, 0.0f );
	ctx->addVertex( 2.5f, 1.0f, 0.0f );

	ctx->setColor( 0.0f, 0.0f, 1.0f );
	ctx->addVertex( 0.5f, 1.0f, 0.0f );

	ctx->endPrimitive();
}

// Basic shapes

/*
*  Vertices are computed as follows:
*     7+------+6
*     /|     /|      y
*    / |    / |      |
*   / 3+---/--+2     |
* 4+------+5 /       *---x
*  | /    | /       /
*  |/     |/       z
* 0+------+1      
*/
void rtdb::loadCube()
{
	rt::Context* ctx = rt::Context::current();

	ctx->beginPrimitive( RT_TRIANGLES );

	// Top
	ctx->setNormal( 0.0f, 1.0f, 0.0f );
	ctx->addVertex( rtdb::CUBE_VERTICES + 4*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 5*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 6*3 );

	ctx->addVertex( rtdb::CUBE_VERTICES + 4*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 6*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 7*3 );

	// Bottom
	ctx->setNormal( 0.0f, -1.0f, 0.0f );
	ctx->addVertex( rtdb::CUBE_VERTICES + 0*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 3*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 2*3 );

	ctx->addVertex( rtdb::CUBE_VERTICES + 0*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 2*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 1*3 );

	// Left
	ctx->setNormal( -1.0f, 0.0f, 0.0f );
	ctx->addVertex( rtdb::CUBE_VERTICES + 4*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 3*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 0*3 );

	ctx->addVertex( rtdb::CUBE_VERTICES + 4*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 7*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 3*3 );

	// Right
	ctx->setNormal( 1.0f, 0.0f, 0.0f );
	ctx->addVertex( rtdb::CUBE_VERTICES + 5*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 2*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 6*3 );

	ctx->addVertex( rtdb::CUBE_VERTICES + 5*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 1*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 2*3 );

	// Front
	ctx->setNormal( 0.0f, 0.0f, 1.0f );
	ctx->addVertex( rtdb::CUBE_VERTICES + 0*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 1*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 5*3 );

	ctx->addVertex( rtdb::CUBE_VERTICES + 0*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 5*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 4*3 );

	// Back
	ctx->setNormal( 0.0f, 0.0f, -1.0f );
	ctx->addVertex( rtdb::CUBE_VERTICES + 3*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 7*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 6*3 );

	ctx->addVertex( rtdb::CUBE_VERTICES + 3*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 6*3 );
	ctx->addVertex( rtdb::CUBE_VERTICES + 2*3 );
	ctx->endPrimitive();
}

void rtdb::loadSphere( uint32 slices, uint32 stacks )
{
	rt::Context* ctx = rt::Context::current();

	uint32 i,j;
	double t1,t2,t3;
	vr::vec3f e,p;

	float phi1 = 0;
	float phi2 = vr::Mathf::TWO_PI;   

	float r  = 1.0f;

	float theta1 = 0;
	float theta2 = vr::Mathf::PI;

	vr::vec3f c( 0.0f, 0.0f, 0.0f );

	for( j = 0; j < stacks/2; j++ )
	{
		t1 = phi1 + j * (phi2 - phi1) / (slices/2);
		t2 = phi1 + (j + 1) * (phi2 - phi1) / (slices/2);

		ctx->beginPrimitive( RT_TRIANGLE_STRIP );

		for (i=0;i <= slices;i++)
		{
			t3 = theta1 + i * (theta2 - theta1) / slices;

			e.x = cos(t1) * cos(t3);
			e.y = sin(t1);
			e.z = cos(t1) * sin(t3);
			p.x = c.x + r * e.x;
			p.y = c.y + r * e.y;
			p.z = c.z + r * e.z;
			ctx->setNormal(e.x,e.y,e.z);
			ctx->setTexCoord(i/(double)slices,2*j/(double)slices, 0.0f);
			ctx->addVertex(p.x,p.y,p.z);

			e.x = cos(t2) * cos(t3);
			e.y = sin(t2);
			e.z = cos(t2) * sin(t3);
			p.x = c.x + r * e.x;
			p.y = c.y + r * e.y;
			p.z = c.z + r * e.z;
			ctx->setNormal(e.x,e.y,e.z);
			ctx->setTexCoord(i/(double)slices,2*(j+1)/(double)slices, 0.0f);
			ctx->addVertex(p.x,p.y,p.z);

		}
		ctx->endPrimitive();
	}
}

void rtdb::loadTeapot()
{
	rt::Context* ctx = rt::Context::current();

	uint32 i = 0;
	while( i < rtdb::NUM_TEAPOT_INDEXES )
	{
		ctx->beginPrimitive( RT_TRIANGLE_STRIP );
		while( rtdb::TEAPOT_INDEXES[i] != rtdb::STRIP_END )
		{
			int32 index = rtdb::TEAPOT_INDEXES[i] * 3;
			ctx->setNormal( rtdb::TEAPOT_NORMALS + index );
			ctx->addVertex( rtdb::TEAPOT_VERTICES + index );
			i++;
		}
		ctx->endPrimitive();
		i++; // skip strip end flag
	}
}

// Default scenes
void rtdb::loadEvenSpaced()
{
	rt::Context* ctx = rt::Context::current();

	for( float z = 0; z < 4.5; z+=1.5 )
	{
		for( float y = 0; y < 4.5; y+=1.5 )
		{
			for( float x = 0; x < 4.5; x+=1.5 )
			{
				ctx->getMatrixStack().pushMatrix();
				ctx->getMatrixStack().translate( x, y, z );
				rtdb::loadTriangle();
				ctx->getMatrixStack().popMatrix();
			}
		}
	}
}

void rtdb::loadMixed()
{
	rt::Context* ctx = rt::Context::current();

	ctx->getMatrixStack().pushMatrix();
	ctx->getMatrixStack().translate( 1.0f, 2.0f, 2.5f );
	rtdb::loadLogo();
	ctx->getMatrixStack().popMatrix();

	ctx->getMatrixStack().pushMatrix();
	ctx->getMatrixStack().translate( 0.0f, 5.0f, 0.0f );
	rtdb::loadCube();
	ctx->getMatrixStack().translate( 5.0f, 0.0f, 0.0f );
	rtdb::loadCube();
	ctx->getMatrixStack().translate( 0.0f, 0.0f, 5.0f );
	rtdb::loadCube();
	ctx->getMatrixStack().translate( -5.0f, 0.0f, 0.0f );
	rtdb::loadCube();
	ctx->getMatrixStack().popMatrix();

	ctx->getMatrixStack().pushMatrix();
	ctx->getMatrixStack().scale( 5.0f, 5.0f, 5.0f );
	rtdb::loadTeapot();
	ctx->getMatrixStack().translate( 5.0f, 0.0f, 0.0f );
	rtdb::loadTeapot();
	ctx->getMatrixStack().translate( 0.0f, 0.0f, 5.0f );
	rtdb::loadTeapot();
	ctx->getMatrixStack().translate( -5.0f, 0.0f, 0.0f );
	rtdb::loadTeapot();
	ctx->getMatrixStack().popMatrix();
}

void rtdb::loadTeapots125()
{
	rt::Context* ctx = rt::Context::current();

	// 125 teapots = 125 * 2256 = 282k triangles
	for( int z = 0; z < 5; ++z )
	{
		for( int y = 0; y < 5; ++y )
		{
			for( int x = 0; x < 5; ++x )
			{
				ctx->getMatrixStack().pushMatrix();
				ctx->getMatrixStack().scale( 5.0f, 5.0f, 5.0f );
				ctx->getMatrixStack().translate( x*2, y*2, z*2 );
				rtdb::loadTeapot();
				ctx->getMatrixStack().popMatrix();
			}
		}
	}
}

// Utility functions

vr::String rtdb::getExtension( const vr::String& filename )
{
	int idx = filename.lastIndexOf( '.' );
	return filename.mid( idx, filename.length() - idx );
}

vr::String rtdb::getFilePath( const vr::String& filename )
{
	int slashPos = vr::max( filename.lastIndexOf( '/' ), filename.lastIndexOf( '\\' ) );

	if( slashPos == -1 )
		return vr::String();

	vr::String str = filename;

	str.remove( slashPos + 1, str.length() - slashPos );
	return str;
}

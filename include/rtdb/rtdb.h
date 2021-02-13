#ifndef _RTDB_H_
#define _RTDB_H_

#include <rt/common.h>

/************************************************************************/
/* Ray Tracing Utility Toolkit                                          */
/************************************************************************/
namespace rtdb {

// Basic shapes
void loadTriangle();
void loadLogo();
void loadCube();
void loadSphere( uint32 slices, uint32 stacks );
void loadTeapot();

// Default scenes
void loadEvenSpaced();
void loadMixed();
void loadTeapots125();

// Utility functions

// Get file extension with dot (i.e. "file.txt" = ".txt")
vr::String getExtension( const vr::String& filename );

// Get file path without file name
vr::String getFilePath( const vr::String& filename );

} // namespace rtdb

#endif // _RTDB_H_

#ifndef _RTDB_CUBE_H_
#define _RTDB_CUBE_H_

namespace rtdb {

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
static const float CUBE_VERTICES[] = {
	-0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,
	-0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f
};

} // namespace rtdb

#endif // _RTDB_CUBE_H_

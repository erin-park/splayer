#ifndef FFMPEG_COORDINATE_H
#define FFMPEG_COORDINATE_H

//     UV Coordinates
//           _________
//  v0(0,0) |         | v3(1,0)
//          |         |
//          |         |
//  v1(0,1) |_________| v2(1,1)
//
//
//   plane vertices
//      _________
//  p0 |         | p3
//     |         |
//     |         |
//  p1 |_________| p2
//

const float squ_vtx[] = {
	-1.0f,  1.0f,  0.0f,		//p0 Left-Top corner
	-1.0f, -1.0f,  0.0f,		//p1 Left-bottom corner
	1.0f, -1.0f,  0.0f,			//p2 Right-bottom corner
	1.0f,  1.0f,  0.0f };		//p3 Right-top corner

// USE GL_TRIANGLE_FAN
const short squ_inx[] = { 0, 1, 2, 3 };	//0-1-2 first triangle
										//0-2-3 second triangle
const float squ_tex[] = {
	0.0f, 0.0f,					//v0 Left-Top corner
	0.0f, 1.0f,					//v1 Left-bottom corner
	1.0f, 1.0f,					//v2 Right-top corner
	1.0f, 0.0f };				//v3 Right-bottom corner

const float skybox_vtx[] = {
    -1,  1,  1,     // (0) Top-left near
     1,  1,  1,     // (1) Top-right near
    -1, -1,  1,     // (2) Bottom-left near
     1, -1,  1,     // (3) Bottom-right near
    -1,  1, -1,     // (4) Top-left far
     1,  1, -1,     // (5) Top-right far
    -1, -1, -1,     // (6) Bottom-left far
     1, -1, -1      // (7) Bottom-right far           
};

//36
const short skybox_inx[] = {
	// front
	1, 3, 0,
	0, 3, 2,
	// back
	4, 6, 5,
	5, 6, 7,
	// left
	0, 2, 4,
	4, 2, 6,
	// right
	5, 7, 1,
	1, 7, 3,
	// top
	5, 1, 4,
	4, 1, 0,
	// bottom
	6, 2, 7,
	7, 2, 3
};

const float cube_vtx[] = {
	// Front face
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,

	// Right face
	1.0f,  1.0f,  1.0f,
	1.0f, -1.0f,  1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f, -1.0f,  1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,

	// Back face
	1.0f,  1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	// Left face
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,

	// Top face
	-1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f,  1.0f,
	1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f,  1.0f,
	1.0f, 1.0f,  1.0f,
	1.0f, 1.0f, -1.0f,

	// Bottom face
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,};

const float cube_normals[] = {
	// Front face
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,

	// Right face
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,

	// Back face
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,

	// Left face
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,

	// Top face
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,

	// Bottom face
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f };

#endif // FFMPEG_COORDINATE_H

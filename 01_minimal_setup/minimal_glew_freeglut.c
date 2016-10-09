/*
The MIT License (MIT)

Copyright (c) 2016 Inês Almeida

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
 * OpenGL Playground - Example of a minimal setup with GLEW and freeGLUT
 *
 * Draws a triangle with a solid color
 *
 * Compiling this example:
 * Linux: gcc minimal_glew_freeglut.c -lGL -lGLEW -lglut -o minimal
 * Mac OS: clang minimal_glew_freeglut.c -framework OpenGL -lglew -framework GLUT -Wno-deprecated -framework Cocoa -o minimal
 *    Important note: Mac has the original glut version that is marked as deprecated, not the improved freeglut
 *
 * Requires OpenGL 3.2 and that GLEW and freeGLUT are installed or provided as includes for compilation
 */

#include <GL/glew.h>
#ifdef __APPLE__
#    include <GLUT/glut.h>
#else
#    include <GL/freeglut.h>
#endif

#include <stdlib.h>
#include <stdio.h>

typedef enum { false, true } bool;

GLuint program;
GLint attr_vpos;


void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program);
	glEnableVertexAttribArray(attr_vpos);
	glVertexAttribPointer(
		attr_vpos, // shader attribute index
		2,         // number of elements per vertex
		GL_FLOAT,  // data type of each element
		GL_FALSE,  // normalized?
		0,         // stride if data is interleaved
		0          // pointer offset to start of data
	);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisableVertexAttribArray(attr_vpos);
	glUseProgram(0);

	glutSwapBuffers(); // glutSwapBuffers makes a glFlush() call
}


int compile_shader(GLuint shader, const char* source, GLenum shader_type)
{
	GLint is_compile_ok = GL_FALSE;

	fprintf(stdout, "Compiling %s Shader...  ", shader_type==GL_VERTEX_SHADER? "Vertex" : "Fragment");

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compile_ok);
	fprintf(stdout, "%s!\n", is_compile_ok? "OK" : "FAILED");

	GLint written, logSize = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
	GLchar buf[logSize];
	glGetShaderInfoLog(shader, logSize, &written, buf);
	if(written) {
		fprintf(stderr, "%s", buf);
		if(logSize != written+1) { // +1 for NULL terminating character
			fprintf(stderr, "... missing %d characters!\n", logSize-written+1);
		}
	}

	if(!is_compile_ok) {
		glDeleteShader(shader);
		return false;
	}

	return true;
}


int init()
{
	// global state
	glClearColor(0.0, 0.0, 0.0, 0.0);

	// compiling shaders
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const char *vs_source =
	"#version 330\n"
	"layout (location = 0) in vec2 v_pos;"
	"void main(void) {"
	"  gl_Position = vec4(v_pos, 0.0, 1.0);"
	"}";
	if(!compile_shader(vs, vs_source, GL_VERTEX_SHADER)) return false;

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fs_source =
	"#version 330\n"
	"out vec4 FragColor;"
	"void main(void) {"
	"  FragColor = vec4(0.0, 1.0, 0.0, 1.0);"
	"}";
	if(!compile_shader(fs, fs_source, GL_FRAGMENT_SHADER)) return false;

	// linking into a program
	GLint is_link_ok = GL_FALSE;
	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &is_link_ok);
	glDetachShader(program, vs);
	glDetachShader(program, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);
	if(!is_link_ok) {
		fprintf(stderr, "Program didn't link\n");
		return false;
	}

	// setting attributes from the application to the vertex shader
	attr_vpos = glGetAttribLocation(program, "v_pos");
	if(attr_vpos == -1) {
		// if this fails and there is no problem in the attribute piping, check if it was optimized out in the shader
		fprintf(stderr, "Setting shader attribute 'v_pos' failed.\n");
		return false;
	}

	// setting up buffers and copying vertex data to the GPU
	// a VAO holds and manages other buffers for vertex data such as VBOs
	GLfloat vertices[] = {
		-0.75, -0.75,
		 0.00,  0.75,
		 0.75, -0.75
	};
	GLuint vao, buffer;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao); // binds to the current context
	glGenBuffers(1, &buffer); // generate ID
	glBindBuffer(GL_ARRAY_BUFFER, buffer); // connects the buffer to the context target
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // transfer data to target

	return true;
}


// callbacks

void key_cb(unsigned char key, int x, int y)
{
	if (key == 27)
		exit(0);
}

// main

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_ALPHA
		#ifdef __APPLE__
		GLUT_3_2_CORE_PROFILE
		#endif
	);
	#ifndef __APPLE__ // apple ships with the old glut, instead of freeglut, and therefore does not have these functions
	glutInitContextVersion(3, 2);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	#endif
	glutInitWindowSize(250, 250);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("OpenGL Test");

	// GLEW
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if(err != GLEW_OK) {
		fprintf(stderr, "GLEW Error: %s\nQuitting...\n", glewGetErrorString(err));
		exit(-1);
	}
	printf("Using OpenGL %s\n", glGetString(GL_VERSION));

	if(!init()) {
		exit(-1);
	}

	// callbacks
	glutKeyboardFunc(key_cb);
	glutDisplayFunc(display);

	// main loop
	glutMainLoop();

	// shutdown
	exit(0);
}

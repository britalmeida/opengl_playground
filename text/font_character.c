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


// gcc font_character.c -I/usr/include/freetype2 -lGL -lGLEW -lglfw -lfreetype -o font_character

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H

GLuint program;
GLint attr_vpos, attr_vtex;
GLuint tex;

GLFWwindow* window;

FT_Library ft_library;
FT_Face ft_face;
uint dpi = 72;
#define FONT_FILE "/usr/share/fonts/TTF/LiberationSans-Regular.ttf"

typedef enum { false, true } bool;

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
	glEnableVertexAttribArray(attr_vtex);
	glVertexAttribPointer(attr_vtex, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*) (2 * 4 * sizeof(float)));
	glBindTexture(GL_TEXTURE_2D, tex);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisableVertexAttribArray(attr_vpos);
	glDisableVertexAttribArray(attr_vtex);
	glUseProgram(0);

	glFlush();
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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	// compiling shaders
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const char *vs_source =
	"#version 330                      \n"
	"layout (location = 0) in vec2 v_pos;"
	"layout (location = 1) in vec2 v_tex;"
	"out vec2 vs_tex_coord;"
	"void main(void) {"
	"  gl_Position = vec4(v_pos, 0.0, 1.0);"
	"  vs_tex_coord = v_tex;"
	"}";
	if(!compile_shader(vs, vs_source, GL_VERTEX_SHADER)) return false;

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fs_source =
	"#version 330                         \n"
	"uniform sampler2D tex;"
	"in vec2 vs_tex_coord;"
	"layout (location = 0) out vec4 color;"
	"void main(void) {"
	"  color = vec4(1.0, 1.0, 1.0, texture(tex, vs_tex_coord).r);"
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
	if(attr_vpos == -1) { fprintf(stderr, "Setting shader attribute failed (v_pos)\n"); return false; }
	attr_vtex = glGetAttribLocation(program, "v_tex");
	if(attr_vtex == -1) { fprintf(stderr, "Setting shader attribute failed (v_tex)\n"); return false; }

	// texture
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	// if we access, from the shader, texture coordinates outside the [0.0 , 1.0] range we get the texel from the edge
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// if it is determined that the texture needs to be 'scaled' when applied,
	// GL_LINEAR gives an average of nearby pixels and GL_NEAREST just the closest one.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, 0);

	// setting up buffers and copying vertex data to the GPU
	// a VAO holds and manages other buffers for vertex data such as VBOs
	GLfloat quad_data[] = {
		// vertices
		-0.75, -0.75,
		 0.75, -0.75,
		 0.75,  0.75,
		-0.75,  0.75,
		// texture coords
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		0.0, 0.0
	};
	GLuint vao, buffer;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao); // binds to the current context
	glGenBuffers(1, &buffer); // generate ID
	glBindBuffer(GL_ARRAY_BUFFER, buffer); // connects the buffer to the context target
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW); // transfer data to target

	return true;
}

void shutdown_glfw_and_exit(int status_code)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(status_code);
}

// callbacks

static void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

static void error_cb(int error, const char* description)
{
	fprintf(stderr, "ERROR: %s\n", description);
}

// freetype

void debug_print_glyph(FT_GlyphSlot glyph)
{
	FT_Int i, j;
	FT_Int width = glyph->bitmap.width;
	FT_Int rows = glyph->bitmap.rows;
	char temp_debug[width+1];
	temp_debug[width] = '\0';

	printf("%d rows, %d width\n", glyph->bitmap.rows, glyph->bitmap.width);

	for(i = 0; i < rows; i++) {
		for(j = 0; j < width; j++) {
			unsigned char b = glyph->bitmap.buffer[i * width + j];
			temp_debug[j] = ( b == 0 ? ' ' : b < 128 ? '+' : '*' );
		}
		printf("%s\n", temp_debug);
	}
}

void init_freetype()
{
	if(FT_Init_FreeType(&ft_library)) {
		fprintf(stderr, "Freetype Error! Quitting...\n");
		shutdown_glfw_and_exit(-1);
	}
	if(FT_New_Face(ft_library, FONT_FILE, 0, &ft_face)) {
		fprintf(stderr, "Freetype could not load face! Quitting...\n");
		shutdown_glfw_and_exit(-1);
	}
	FT_Set_Char_Size(ft_face, 0, (46 * 64), dpi, dpi);
	FT_Set_Pixel_Sizes(ft_face, 0, 46);
}

void load_char_texture()
{
	FT_Load_Char(ft_face, 'a', FT_LOAD_RENDER);
	FT_GlyphSlot slot = ft_face->glyph;
	if(!slot) {
		fprintf(stderr, "Freetype could not get Glyph! Quitting...\n");
		shutdown_glfw_and_exit(-1);
	}

	debug_print_glyph(slot);

	FT_Bitmap *bmp = &slot->bitmap;
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(
		GL_TEXTURE_2D,     // target
		0,                 // mipmap level
		GL_RED,            // internal format
		bmp->width,        // width
		bmp->rows,         // height
		0,                 // legacy border, must be 0
		GL_RED,            // format of the pixel data
		GL_UNSIGNED_BYTE,  // data type of the pixel data
		bmp->buffer        // pointer to the data
	);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// main

int main(int argc, char** argv)
{
	glfwSetErrorCallback(error_cb);

	// GLFW init
	if(!glfwInit()) {
		fprintf(stderr, "GLFW Error: Failed to initialize\nQuitting...\n");
		exit(-1);
	}
	printf("Using GLFW %s\n", glfwGetVersionString());

	// Context creation
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	window = glfwCreateWindow(350, 350, "Font Rendering Test", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(-1);
	}

	glfwSetKeyCallback(window, key_cb);

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if(err != GLEW_OK) {
		fprintf(stderr, "GLEW Error: %s\nQuitting...\n", glewGetErrorString(err));
		shutdown_glfw_and_exit(-1);
	}
	printf("Using OpenGL %s\n", glGetString(GL_VERSION));

	glfwSwapInterval(1);

	if(!init()) {
		shutdown_glfw_and_exit(-1);
	}

	// freetype
	init_freetype();
	load_char_texture();

	// main loop
	while (!glfwWindowShouldClose(window))
	{
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// shut down
	shutdown_glfw_and_exit(0);
}

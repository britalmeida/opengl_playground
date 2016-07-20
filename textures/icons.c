/*
 * OpenGL Playground - Example of displaying a chosen icon from a set in a texture
 *
 * This example loads the icon set from disk as a single texture 'icons.png'
 * The icon set texture must be square and divided into regions of the exact same size to hold each icon.
 * How many icons fit in a side of the texture must be previously known.
 * The texture is transfered to the GPU only once at startup in load_texture().
 * 
 * The icon is selected by ID, starting at 0 from the bottom left corner of the texture.
 * The chosen icon ID and the icons per side are passed in as a uniform to the VS.
 * The VS then computes which region of the texture should be mapped by getting the correct icon square in texture coordinates.
 * 
 * The left and right arrow keys are hooked up to change the icon id.
 * In a real application, it is recommended to have an enum giving meaningful names to each icon. eg:
 * enum {
 *   ICON_OK, //0
 *   ICON_CANCEL,
 *   ...
 * that can then be sent to a draw_icon(int icon_id) that sets the uniform for the shaders.
 *
 * Compiling this example:
 * Linux: gcc lodepng.c icons.c -lGL -lGLEW -lglfw -DLODEPNG_NO_COMPILE_CPP -o icons
 *
 * Requires OpenGL 3.2 and that GLEW and GLFW are installed or provided as includes for compilation
 * Requires the included LodePNG library: http://lodev.org/lodepng/
 */

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>

#include "lodepng.h"

GLuint program;
GLint attr_vpos, attr_vtex, attr_iconid;
GLuint tex;

GLFWwindow* window;

typedef enum { false, true } bool;


GLuint icon_id[2] = {0, 4}; // icon id is '0'. there are '4' icons per side of the texture

void load_icons_texture()
{
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	// if we access, from the shader, texture coordinates outside the [0.0 , 1.0] range we get the texel from the edge
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// if it is determined that the texture needs to be 'scaled' when applied,
	// GL_LINEAR gives an average of nearby pixels and GL_NEAREST just the closest one.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load png image from disk. uses lodepng
	unsigned int error;
	unsigned char* image_data;
	GLuint width, height;
	GLuint num_icons_per_side = 4;
	error = lodepng_decode32_file(&image_data, &width, &height, "icons.png");
	if(error) fprintf(stderr, "Error loading image file %u: %s\n", error, lodepng_error_text(error));

	glTexImage2D(
		GL_TEXTURE_2D,     // target
		0,                 // mipmap level
		GL_RGBA,           // internal format
		width, height,     // width, height
		0,                 // legacy border, must be 0
		GL_RGBA,           // format of the pixel data
		GL_UNSIGNED_BYTE,  // data type of the pixel data
		image_data         // pointer to the data
	);
	glBindTexture(GL_TEXTURE_2D, 0);
	free(image_data);
}

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
	glUniform2uiv(attr_iconid, 1, icon_id); // send '1' of type uint vec2, given in 'icon_id', to the uniform attribute location 'attr_iconid'
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
	"#version 330\n"
	// icon_id.x has the icon id, and y how many icons the texture has per side
	// eg. (6, 4): icon number 6 is located at row 1, col 2 (starting at 0), given by 6/4 = 1, remainder 2
	"uniform uvec2 icon_id;\n"
	"layout (location = 0) in vec2 v_pos;\n"
	"layout (location = 1) in vec2 v_tex;\n"
	"out vec2 vs_tex_coord;\n"
	"void main(void) {\n"
	"  float square_side = float(1)/float(icon_id.y);\n"
	"  gl_Position = vec4(v_pos, 0.0, 1.0);\n"
	"  vs_tex_coord = vec2(\n"
		// col = remainder (6%4 = 2), then multiplied by square_side to give the left corner in tex coords, + 0 or 1 square side for the right corner
	"    mod(icon_id.x, icon_id.y) * square_side + v_tex.x * square_side,\n"
		// row = quotient (6/4 = 1), with 0 being up, so invert, then multiplied by square_side to give the top corner in tex coords, + 0 or 1 for the bottom
	"    (icon_id.y-uint(1) - icon_id.x/icon_id.y) * square_side + v_tex.y * square_side);\n"
	"}\n";
	if(!compile_shader(vs, vs_source, GL_VERTEX_SHADER)) return false;

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fs_source =
	"#version 330\n"
	"uniform sampler2D tex;\n"
	"in vec2 vs_tex_coord;\n"
	"layout (location = 0) out vec4 color;\n"
	"void main(void) {\n"
	"  color = texture(tex, vs_tex_coord);\n"
	"}\n";
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
	// if this fails and there is no problem in the attribute piping, check if it was optimized out in the shader
	if(attr_vpos == -1) { fprintf(stderr, "Setting shader attribute 'v_pos' failed.\n"); }
	attr_vtex = glGetAttribLocation(program, "v_tex");
	if(attr_vtex == -1) { fprintf(stderr, "Setting shader attribute 'v_tex' failed.\n"); }
	attr_iconid = glGetUniformLocation(program, "icon_id");

	// texture
	load_icons_texture();

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

	if(key >= '0' && key <= '9' && action == GLFW_PRESS) {
		icon_id[0] = key-'0';
	}
	else if(key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		icon_id[0]++;
	}
	else if(key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		icon_id[0]--;
	}
}

static void error_cb(int error, const char* description)
{
	fprintf(stderr, "ERROR: %s\n", description);
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

	window = glfwCreateWindow(350, 350, "Icon from Texture Set", NULL, NULL);
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

	// main loop
	while (!glfwWindowShouldClose(window))
	{
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// shutdown
	shutdown_glfw_and_exit(0);
}

/**
 * Clean C example / sample program of OpenGL ES2 using fbdev /dev/fb0 device
 *
 * This program display a red fullscreen quad.
 * Tested on NanoPI Fire 3 hardware (Mali 400 GPU) with 's5p6818-sd-friendlycore-xenial-4.4-arm64-20190128.img' image file (provided by friendlyarm)
 *
 * Compiled with : gcc fullquad_gles2_fbdev.c -lEGL -o fullquad_gles2_fbdev
 *
 * By Julien Verneuil - 24/03/2019
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <linux/fb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

const GLfloat quadData[] = {
    // vertices
    -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f,
    // UV
	0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f
};

const char *simpleVs = "attribute mediump vec3 vp; \
    attribute mediump vec2 vu; \
    varying mediump vec2 uv; \
    void main() { \
        uv = vu; \
        gl_Position = vec4(vp, 1.0); \
    }";

const char *simpleFs = "varying mediump vec2 uv; \
    void main() { \
        gl_FragColor = vec4(1.0, 0., 0., 0.); \
    }";

void printShaderLog(GLuint obj, int type) {
	static char log[16384];

	if (type == 0) {
		glGetProgramInfoLog(obj, 16384, 0, log);
	} else if (type == 1) {
		glGetShaderInfoLog(obj, 16384, 0, log);
	}
	log[16383] = 0;

	fprintf(stderr, "printShaderLog : \n%s\n\n", log);
}
 
GLuint loadShader(const char *shader_source, GLenum type) {
    GLint shader, status;

    shader = glCreateShader(type);
 
    glShaderSource(shader, 1, &shader_source, NULL);
    glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		fprintf(stderr, "loadShader : Failed to compile shader\n");

		printShaderLog(shader, 1);

		glDeleteShader(shader);
		
        return 0;
	}

    return shader;
}

void render(GLint vbo, GLint vp, GLint vu) {
    // render the fullscreen quad
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*) (12 * sizeof(GLfloat)));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint setupFullscreenQuad() {
	GLuint vbo = 0, vao = 0;

    int data_count = 12;

    glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Vertices + UV
	glBufferData(GL_ARRAY_BUFFER, (data_count + (data_count / 3) * 2) * sizeof(GLfloat), &quadData[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vbo;
}

int keep_running = 1;
void sigintHandler(int dummy) {
    keep_running = 0;
}
 
int main(int argc, char **argv) {
    signal(SIGINT, sigintHandler);

    // framebuffer initialization
    struct fb_var_screeninfo fb_var;

    int fd = open("/dev/fb0", O_RDWR);
    if (fd == -1) {
        printf("open /dev/fb0 failed\n");
        return 0;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &fb_var)) {
        printf("ioctl FBIOGET_VSCREENINFO failed\n");

        close(fd);
        return 0;
    }
 
    // EGL initialization
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;

    // didn't need it but maybe useful on some platforms ?
    setenv("EGL_PLATFORM", "fbdev", 0);
    setenv("FRAMEBUFFER", "/dev/fb0", 0);
 
    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY) {
        printf("eglGetDisplay failed : EGL_NO_DISPLAY\n");

        close(fd);
        return 1;
    }
 
    if (!eglInitialize(egl_display, NULL, NULL)) {
        printf("eglInitialize failed\n");

        close(fd);
        return 1;
    }
 
    EGLint attr[] = {
        /*EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 16,*/
        EGL_BUFFER_SIZE, 16,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
 
    EGLConfig eglconf;
    EGLint num_config;
    if (!eglChooseConfig( egl_display, attr, &eglconf, 1, &num_config)) {
        printf("eglChooseConfig failed\n");
        eglTerminate(egl_display);
        close(fd);
        return 1;
    }
 
    EGLint ctxattr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    egl_context = eglCreateContext(egl_display, eglconf, EGL_NO_CONTEXT, ctxattr);
    if (egl_context == EGL_NO_CONTEXT) {
        printf("eglCreateContext failed : EGL_NO_CONTEXT\n");
        eglTerminate(egl_display);
        close(fd);
        return 1;
    }

    const EGLNativeWindowType native_win = (EGLNativeWindowType) NULL;
    egl_surface = eglCreateWindowSurface(egl_display, eglconf, native_win, NULL);
    if (egl_surface == EGL_NO_SURFACE) {
        printf("eglCreateWindowSurface failed : EGL_NO_SURFACE\n");
        eglDestroyContext(egl_display, egl_context);
        eglTerminate(egl_display);
        close(fd);
        return 1;
    }
 
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
 
    // OpenGL
    // load & setup shaders
    GLuint vertexShader = loadShader(simpleVs, GL_VERTEX_SHADER);
    GLuint fragmentShader = loadShader(simpleFs, GL_FRAGMENT_SHADER);
    if (vertexShader == 0 || fragmentShader == 0) {
        goto quit_with_error;
    }
 
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
 
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
 
    GLint vp, vu;
    vp = glGetAttribLocation(shaderProgram , "vp");
    vu = glGetAttribLocation(shaderProgram , "vu");
    if (vp < 0 || vu < 0) {
        printf("eglCreateWindowSurface failed : EGL_NO_SURFACE\n");
        goto quit_with_error;
    }

    glViewport(0, 0, fb_var.xres, fb_var.yres);
    glClearColor(0, 0, 0, 1);

    GLint quadVbo = setupFullscreenQuad();
 
    // render loop
    while (keep_running) {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        render(quadVbo, vp, vu);

        eglSwapBuffers(egl_display, egl_surface);
    }

    // bye
    goto quit;

quit_with_error:
    return 1;

quit:
    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);

    close(fd);
 
    return 0;
}
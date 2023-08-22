#include <iostream>
#include <mutex>
#include <thread>

#include <glad/gl.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

// #define USE_GLES

struct GLContext {
  SDL_Window *window;
  SDL_GLContext context;
  std::mutex mutex;
};

void checkGLErrors() {
  GLenum error;

  while (error = glGetError(), error != GL_NO_ERROR) {
    const char *string;

    switch (error) {
    case GL_NO_ERROR:
      string = "GL_NO_ERROR";
      break;
    case GL_INVALID_ENUM:
      string = "GL_INVALID_ENUM";
      break;
    case GL_INVALID_VALUE:
      string = "GL_INVALID_VALUE";
      break;
    case GL_INVALID_OPERATION:
      string = "GL_INVALID_OPERATION";
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      string = "GL_INVALID_FRAMEBUFFER_OPERATION";
      break;
    case GL_OUT_OF_MEMORY:
      string = "GL_OUT_OF_MEMORY";
      break;
    default:
      continue;
    }

    std::cout << string << std::endl;
  }
}

class GLContextLock {
private:
  GLContext *mContext;
  inline static std::thread::id lastThreadID;

public:
  GLContextLock(GLContext &context) : mContext(&context) {
    mContext->mutex.lock();
    SDL_GL_MakeCurrent(mContext->window, mContext->context);

    std::thread::id id = std::this_thread::get_id();

    if (id != lastThreadID) {
      std::cout << "MakeCurrent(" << mContext->window << ", "
                << mContext->context << "); Thread ID: " << id << std::endl;
      lastThreadID = id;
    }
  }

  ~GLContextLock() {
    checkGLErrors();
    SDL_GL_MakeCurrent(nullptr, nullptr);
    mContext->mutex.unlock();
  }

public:
  GLContextLock &operator=(const GLContextLock &) = delete;
};

GLContext createContext(SDL_Window *window) {
  SDL_GLContext context = SDL_GL_CreateContext(window);
  gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
  SDL_GL_MakeCurrent(nullptr, nullptr);
  return {window, context, {}};
}

void deleteContext(GLContext &context) {
  SDL_GL_DeleteContext(context.context);
}

GLuint vao;
GLuint vbo;
GLuint shaderProgram;

void initialize(GLContext &context) {
  GLContextLock lock(context);

  float vertices[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f};

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  const char *vertexShaderSource = R"(#version 330 core
    layout (location = 0) in vec2 aPos;
    void main() {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    })";

  const char *fragmentShaderSource = R"(#version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 0.5, 0.2, 1.0);
    })";

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
  glCompileShader(vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
  glCompileShader(fragmentShader);

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
}

int main(int argv, char **args) {
  SDL_SetMainReady();
  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

#ifndef USE_GLES
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif

  SDL_Window *window =
      SDL_CreateWindow("gl_multithread_makecurrent", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);

  GLContext context = createContext(window);

  // Initialize resources in another thread...
  std::thread thread([&] { initialize(context); });
  thread.join();

  bool quit = false;
  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        quit = true;
      }
    }

    {
      GLContextLock lock(context);

      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      glUseProgram(shaderProgram);

      glBindVertexArray(vao);
      glDrawArrays(GL_TRIANGLES, 0, 3);

      SDL_GL_SwapWindow(window);
    }
  }

  {
    GLContextLock lock(context);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
  }

  deleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
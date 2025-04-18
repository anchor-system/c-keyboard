// graphics stuff
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "linmath.h"

#include <stdio.h>
#include <stdlib.h>

#include "RtMidi.h"
#include <cstdlib>
#include <iostream>

// Platform-dependent sleep routines.
#if defined(_WIN32)
#include <windows.h>
#define SLEEP(milliseconds) Sleep((DWORD)milliseconds)
#else // Unix variants
#include <unistd.h>
#define SLEEP(milliseconds) usleep((unsigned long)(milliseconds * 1000.0))
#endif

// This function should be embedded in a try/catch block in case of
// an exception.  It offers the user a choice of MIDI ports to open.
// It returns false if there are no ports available.
bool choose_midi_port(RtMidiOut *rtmidi);

RtMidi::Api choose_midi_api();

RtMidiOut *midiout = 0;
std::vector<unsigned char> message;

bool super_key_pressed = false;
bool transpose_combo_pressed = false;

// back to graphics

static const struct {
  float x, y;
  float r, g, b;
} vertices[3] = {{-0.6f, -0.4f, 1.f, 0.f, 0.f},
                 {0.6f, -0.4f, 0.f, 1.f, 0.f},
                 {0.f, 0.6f, 0.f, 0.f, 1.f}};

static const char *vertex_shader_text =
    "#version 110\n"
    "uniform mat4 MVP;\n"
    "attribute vec3 vCol;\n"
    "attribute vec2 vPos;\n"
    "varying vec3 color;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
    "    color = vCol;\n"
    "}\n";

static const char *fragment_shader_text =
    "#version 110\n"
    "varying vec3 color;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n";

static void error_callback(int error, const char *description) {
  fprintf(stderr, "Error: %s\n", description);
}

const int c_4 = 60;
int transpose = 0;
int lowest_note = -1;
void set_lowest_note(int transpose) { lowest_note = 60 - 2 * 12 + transpose; };

int glfw_top_row_numbers[12] = {
    GLFW_KEY_GRAVE_ACCENT,
    GLFW_KEY_1,
    GLFW_KEY_2,
    GLFW_KEY_3,
    GLFW_KEY_4,
    GLFW_KEY_5,
    GLFW_KEY_6,
    GLFW_KEY_7,
    GLFW_KEY_8,
    GLFW_KEY_9,
    GLFW_KEY_0,
    GLFW_KEY_MINUS,
};

int glfw_key_map[48] = {

    GLFW_KEY_LEFT_SHIFT,
    GLFW_KEY_Z,
    GLFW_KEY_X,
    GLFW_KEY_C,
    GLFW_KEY_V,
    GLFW_KEY_B,
    GLFW_KEY_N,
    GLFW_KEY_M,
    GLFW_KEY_COMMA,
    GLFW_KEY_PERIOD,
    GLFW_KEY_SLASH,
    GLFW_KEY_RIGHT_SHIFT,

    GLFW_KEY_CAPS_LOCK,
    GLFW_KEY_A,
    GLFW_KEY_S,
    GLFW_KEY_D,
    GLFW_KEY_F,
    GLFW_KEY_G,
    GLFW_KEY_H,
    GLFW_KEY_J,
    GLFW_KEY_K,
    GLFW_KEY_L,
    GLFW_KEY_SEMICOLON,
    GLFW_KEY_APOSTROPHE,

    GLFW_KEY_TAB,
    GLFW_KEY_Q,
    GLFW_KEY_W,
    GLFW_KEY_E,
    GLFW_KEY_R,
    GLFW_KEY_T,
    GLFW_KEY_Y,
    GLFW_KEY_U,
    GLFW_KEY_I,
    GLFW_KEY_O,
    GLFW_KEY_P,
    GLFW_KEY_LEFT_BRACKET,

    GLFW_KEY_GRAVE_ACCENT,
    GLFW_KEY_1,
    GLFW_KEY_2,
    GLFW_KEY_3,
    GLFW_KEY_4,
    GLFW_KEY_5,
    GLFW_KEY_6,
    GLFW_KEY_7,
    GLFW_KEY_8,
    GLFW_KEY_9,
    GLFW_KEY_0,
    GLFW_KEY_MINUS,
};

int convert_key_to_note(int pressed_key) {
  for (int i = 0; i < 48; i++) {
    if (glfw_key_map[i] == pressed_key) {
      return lowest_note + i;
    }
  }
  return -1; // invalid key
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  // if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
  //     glfwSetWindowShouldClose(window, GLFW_TRUE);
  //
  if (key == GLFW_KEY_SPACE) {
    if (action == GLFW_PRESS) {
      super_key_pressed = true;
      printf("super key pressed\n");
    } else if (action == GLFW_RELEASE) {
      printf("super key released\n");
      super_key_pressed = false;
    }
  }

  if (key == GLFW_KEY_T) {
    if (action == GLFW_PRESS) {
      printf("t pressed\n");
      if (super_key_pressed) {
        printf("transpose_combo_pressed");
        transpose_combo_pressed = true;
      }
    } else if (action == GLFW_RELEASE) {
      printf("t released\n");
      transpose_combo_pressed = false;
    }
  }

  if (transpose_combo_pressed) {
    printf("starting search\n");
    for (int i = 0; i < 12; i++) {
      if (key == glfw_top_row_numbers[i]) {
        printf("found\n");
        if (action == GLFW_PRESS) {
          printf("set transpose to %d", i);
          set_lowest_note(i);
        }
      }
    }
  }

  if (!super_key_pressed) { // don't play notes if we're issuing a command
    int note = convert_key_to_note(key);
    if (note != -1) {
      if (action == GLFW_PRESS) {
        // Note On: 144, 64, 90
        message[0] = 144; // on
        message[1] = note;
        message[2] = 75; // vel
        midiout->sendMessage(&message);
      } else if (action == GLFW_RELEASE) {
        // Note Off: 128, 64, 40
        message[0] = 128; // off
        message[1] = note;
        message[2] = 40; // vel
        midiout->sendMessage(&message);
      }
    }
  }
}

int main(void) {

  set_lowest_note(0);

  // RtMidiOut constructor
  try {
    midiout = new RtMidiOut(choose_midi_api());
  } catch (RtMidiError &error) {
    error.printMessage();
    exit(EXIT_FAILURE);
  }

  // Call function to select port.
  try {
    if (choose_midi_port(midiout) == false)
      goto cleanup;
  } catch (RtMidiError &error) {
    error.printMessage();
    goto cleanup;
  }

  // Send out a series of MIDI messages.

  // Program change: 192, 5
  message.push_back(192);
  message.push_back(5);
  midiout->sendMessage(&message);

  // Control Change: 176, 7, 100 (volume)
  message[0] = 176;
  message[1] = 7;
  message.push_back(100);
  midiout->sendMessage(&message);

  GLFWwindow *window;
  GLuint vertex_buffer, vertex_shader, fragment_shader, program;
  GLint mvp_location, vpos_location, vcol_location;

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  window = glfwCreateWindow(100, 100, "c-keyboard", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetKeyCallback(window, key_callback);

  glfwMakeContextCurrent(window);
  gladLoadGL(glfwGetProcAddress);
  glfwSwapInterval(1);

  // NOTE: OpenGL error checks have been omitted for brevity

  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader);

  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  mvp_location = glGetUniformLocation(program, "MVP");
  vpos_location = glGetAttribLocation(program, "vPos");
  vcol_location = glGetAttribLocation(program, "vCol");

  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                        sizeof(vertices[0]), (void *)0);
  glEnableVertexAttribArray(vcol_location);
  glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
                        sizeof(vertices[0]), (void *)(sizeof(float) * 2));

  while (!glfwWindowShouldClose(window)) {
    float ratio;
    int width, height;
    mat4x4 m, p, mvp;

    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float)height;

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    mat4x4_identity(m);
    mat4x4_rotate_Z(m, m, (float)glfwGetTime());
    mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_mul(mvp, p, m);

    glUseProgram(program);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat *)mvp);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Clean up midi
cleanup:
  delete midiout;

  return 0;

  glfwDestroyWindow(window);

  glfwTerminate();
  exit(EXIT_SUCCESS);
}

bool choose_midi_port(RtMidiOut *rtmidi) {
  std::cout << "\nWould you like to open a virtual output port? [y/N] ";

  std::string keyHit;
  std::getline(std::cin, keyHit);
  if (keyHit == "y") {
    rtmidi->openVirtualPort();
    return true;
  }

  std::string portName;
  unsigned int i = 0, nPorts = rtmidi->getPortCount();
  if (nPorts == 0) {
    std::cout << "No output ports available!" << std::endl;
    return false;
  }

  if (nPorts == 1) {
    std::cout << "\nOpening " << rtmidi->getPortName() << std::endl;
  } else {
    for (i = 0; i < nPorts; i++) {
      portName = rtmidi->getPortName(i);
      std::cout << "  Output port #" << i << ": " << portName << '\n';
    }

    do {
      std::cout << "\nChoose a port number: ";
      std::cin >> i;
    } while (i >= nPorts);
  }

  std::cout << "\n";
  rtmidi->openPort(i);

  return true;
}

RtMidi::Api choose_midi_api() {
  std::vector<RtMidi::Api> apis;
  RtMidi::getCompiledApi(apis);

  if (apis.size() <= 1)
    return RtMidi::Api::UNSPECIFIED;

  std::cout << "\nAPIs\n  API #0: unspecified / default\n";
  for (size_t n = 0; n < apis.size(); n++)
    std::cout << "  API #" << apis[n] << ": "
              << RtMidi::getApiDisplayName(apis[n]) << "\n";

  std::cout << "\nChoose an API number: ";
  unsigned int i;
  std::cin >> i;

  std::string dummy;
  std::getline(std::cin, dummy); // used to clear out stdin

  return static_cast<RtMidi::Api>(i);
}

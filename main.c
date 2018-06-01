#include <stdio.h>

#ifdef DEBUG
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "read_file.c"
#include "shaders.h.out"
#include "unreeeal_superhero_3.xm.c"
#include "xmplayer.c"
#endif

char **SCENES[] = {&PALLOSCENE_FRAG, &TESTI_FRAG};

void play(void *d, uint8_t *stream, int len) {
    xm_generate_samples((xm_context_t*)d, (float*)stream, (len/8));
}

void music() {
    xm_context_t *xm;
    xm_create_context_safe(&xm, UNREEEAL_SUPERHERO_3_XM, UNREEEAL_SUPERHERO_3_XM_LEN, 48000);

    SDL_AudioSpec want, have;
    want.freq = 48000;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 2048;
    want.callback = play;
    want.userdata = (void*)xm;

    uint32_t dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    SDL_PauseAudioDevice(dev, 0);
}

unsigned compile_shader(unsigned type, char *src) {
    unsigned s = glCreateShader(type);

    // add correct header to shader
    char *full_src[2];
    switch (type) {
        case 0x8B31: // vertex shader
            full_src[0] = "#version 330\n"
                "precision mediump float;\n"
                "layout (location = 0) in vec3 a_pos;\n"
                "layout (location = 1) in vec2 a_texturePos;\n"
                "out vec3 v_pos;\n"
                "out vec2 v_texturePos;\n";
            break;
        case 0x8B30: // fragment shader
            full_src[0] = "#version 330\n"
                "precision mediump float;\n"
                "uniform float u_time;\n"
                "out vec4 fragColor;\n"
                "in vec2 v_texturePos;\n";
            break;
        default:
            full_src[0] = "#version 330\n";
    }
#ifdef DEBUG
    read_file_to_str(&full_src[1], src);
#else
    full_src[1] = src;
#endif

    glShaderSource(s, 2, (const char**)full_src, NULL);
    glCompileShader(s);
    int success; char info[512];
    glGetShaderiv(s, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(s, 512, NULL, info);
        printf("%s", info);
    }
    return s;
}

// Params are filenames in debug builds, but shader source code otherwise
unsigned link_program(char *vertex_src, char *fragment_src) {
    unsigned p = glCreateProgram();
    glAttachShader(p, compile_shader(GL_VERTEX_SHADER, vertex_src));
    glAttachShader(p, compile_shader(GL_FRAGMENT_SHADER, fragment_src));
    glLinkProgram(p);
    int success; char info[512];
    glGetProgramiv(p, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(p, 512, NULL, info);
        printf("%s", info);
        return 0;
    }
    return p;
}

unsigned create_buf(unsigned target, size_t size, const void *data) {
    unsigned buf;
    glGenBuffers(1, &buf);
    glBindBuffer(target, buf);
    glBufferData(target, size, data, GL_STATIC_DRAW);
    glBindBuffer(target, 0);
    return buf;
}

void render(unsigned program, unsigned vertex_array) {
    glUseProgram(program);

    unsigned u_time = glGetUniformLocation(program, "u_time");
    glUniform1f(u_time, SDL_GetTicks()/1000.);

    glBindVertexArray(vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    // MAJOR_VERSION = 17, MINOR_VERSION = 18, PROFILE_MASK = 21
    SDL_GL_SetAttribute(17, 3);
    SDL_GL_SetAttribute(18, 3);
    // SDL_GL_CONTEXT_PROFILE_CORE == 0x1
    SDL_GL_SetAttribute(21, 1);

    SDL_Window *w = SDL_CreateWindow("testi", 0, 0, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GL_CreateContext(w); // SDL_GL_LoadLibrary, SDL_GL_MakeCurrent?

    // Load OpenGL functions using SDL_GetProcAddress if needed
#ifdef DEBUG
    glewInit();
#endif

    // Compile and link shaders
    unsigned scene = 0;
    unsigned s = link_program(TRIVIAL_VERT, *SCENES[scene]);
    if (!s) {
        return EXIT_FAILURE;
    }

    // Generate a VBO
    float const vertices[] = {
        -1., -1., 0., 0., 0.,
        1., -1., 0., 1., 0.,
        1., 1., 0., 1., 1.,
        1., 1., 0., 1., 1.,
        -1., 1., 0., 0., 1.,
        -1., -1., 0., 0., 0.
    };

    unsigned buf = create_buf(GL_ARRAY_BUFFER, sizeof(vertices), vertices);

    // Generate the VAO for the shader quad
    unsigned vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    size_t sf = sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 5 * sf, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, 0, 5 * sf, (void*)(3*sf));
    glEnableVertexAttribArray(1);

    // Start playing music
    music();

    SDL_Event e;
    while(1) {
        //glClear(GL_COLOR_BUFFER_BIT);

        render(s, vertex_array);

        SDL_GL_SwapWindow(w);

        SDL_PollEvent(&e);
#ifdef DEBUG
        if (e.type == SDL_QUIT) {
            break;
        } else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q) {
                break;
            } else if (e.key.keysym.sym == SDLK_r) {
                s = link_program(TRIVIAL_VERT, *SCENES[scene]);
            } else if (e.key.keysym.sym == SDLK_n) {
                scene = (scene + 1) % (sizeof(SCENES)/sizeof(SCENES[0]));
                s = link_program(TRIVIAL_VERT, *SCENES[scene]);
            }
        }
#else
        if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN) break;
#endif
    }

    return 0;
}

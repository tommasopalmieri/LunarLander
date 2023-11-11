

/**
* Author: Tommaso Palmieri
* Assignment: Lunar Lander
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


// Please note that the assets were taken from https://mattwalkden.itch.io/lunar-battle-pack
// I was hoping they were going to look better than they actually do. 
// The rocket image was taken from https://www.pngall.com/rocket-png/, by PNG All, CC-BY-NC
// the "fire" under the rocket was taken from lunar-battle-pack, and attached to the rocket.


// git repo: https://github.com/tommasopalmieri/LunarLander


#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define NUMBER_OF_ENEMIES 3
#define FIXED_TIMESTEP 0.0166666f
#define ACC_OF_GRAVITY -0.5f
#define LEFT_ACCELERATION -7.5f
#define RIGHT_ACCELERATION 7.5f
#define PLATFORM_COUNT 6
#define FAILURE_PLATFORMS 6





#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ————— STRUCTS AND ENUMS —————//
struct GameState
{
    Entity* player;
    Entity* platforms;
    Entity* failure_platforms;
    int gameStatus;
};

// ————— CONSTANTS ————— //
const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.0922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const char  SPRITESHEET_FILEPATH[] = "assets/rocket_0-01.png",
PLATFORM_FILEPATH[] = "assets/Surface_Layer1.png",
FAILURE_PLATFORM[] = "assets/Surface_Layer2.png";

const int NUMBER_OF_TEXTURES = 1;  // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;  // this value MUST be zero

// ————— VARIABLES ————— //
GameState g_game_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;


// FUEL Mechanism
// as far as requirement #2 is concerned: it will not come to a stop if an accelleration is applied, assuming no friction. 
// I don't think it can come to a stop if your request is to apply an accelleration with the arrow keys. 

int avaliable_fuel = 1000;

void accLeft() {
    if (avaliable_fuel > 0) {
        g_game_state.player->set_acceleration(glm::vec3(LEFT_ACCELERATION, g_game_state.player->get_acceleration().y, 0.0f));
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->LEFT];
        avaliable_fuel--;
        
    }

}

void accRight() {
    if (avaliable_fuel > 0) {
        g_game_state.player->set_acceleration(glm::vec3(RIGHT_ACCELERATION, g_game_state.player->get_acceleration().y, 0.0f));
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->LEFT];
        avaliable_fuel--;
    }


}


void accUp() {
    if (avaliable_fuel > 0) {
        g_game_state.player->set_acceleration(glm::vec3(0.0f, g_game_state.player->get_acceleration().y + 0.05f, 0.0f));
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->UP];
        avaliable_fuel--;
    }

}


// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Lunar Lander",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ————— PLAYER ————— //
    // Existing
    g_game_state.player = new Entity();
    g_game_state.player->set_position(glm::vec3(0.0f,3.0f, 0.0f));;
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY * 0.1, 0.0f));
    g_game_state.player->set_speed(1.0f);
    g_game_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);

    // Walking
    g_game_state.player->m_walking[g_game_state.player->LEFT] = new int[4] { 1, 5, 9, 13 };
    g_game_state.player->m_walking[g_game_state.player->RIGHT] = new int[4] { 3, 7, 11, 15 };
    g_game_state.player->m_walking[g_game_state.player->UP] = new int[4] { 2, 6, 10, 14 };
    g_game_state.player->m_walking[g_game_state.player->DOWN] = new int[4] { 0, 4, 8, 12 };

    g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->DOWN];  // start George looking right
    g_game_state.player->m_animation_frames = 4;
    g_game_state.player->m_animation_index = 0;
    g_game_state.player->m_animation_time = 0.0f;
    g_game_state.player->m_animation_cols = 4;
    g_game_state.player->m_animation_rows = 4;
    g_game_state.player->set_height(0.5f);
    g_game_state.player->set_width(0.5f);

    // Jumping
    g_game_state.player->m_jumping_power = 3.0f;

    // ————— PLATFORM ————— //
    g_game_state.platforms = new Entity[PLATFORM_COUNT];
    g_game_state.failure_platforms = new Entity[FAILURE_PLATFORMS];

    int i_success_platforms = 0;
    int i_failure_platforms = 0;

    for (int i = 0; i < PLATFORM_COUNT+FAILURE_PLATFORMS; i++)
    {
        if (i % 2 == 0) {
            g_game_state.platforms[i_success_platforms].m_texture_id = load_texture(PLATFORM_FILEPATH);
            g_game_state.platforms[i_success_platforms].set_position(glm::vec3(i - 4.5f, -3.5f, 0.0f));
            g_game_state.platforms[i_success_platforms].update(0.0f, NULL, 0, NULL, 0);
            i_success_platforms++;
        }
        else {
            g_game_state.failure_platforms[i_failure_platforms].m_texture_id = load_texture(FAILURE_PLATFORM);
            g_game_state.failure_platforms[i_failure_platforms].set_position(glm::vec3(i - 4.5f, -3.5f, 0.0f));
            g_game_state.failure_platforms[i_failure_platforms].update(0.0f, NULL, 0, NULL, 0);
            i_failure_platforms++;
        }

    }

    // ————— GENERAL ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            case SDLK_SPACE:
                // Jump
                if (g_game_state.player->m_collided_bottom) g_game_state.player->m_is_jumping = true;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        accLeft();
        //g_game_state.player->set_acceleration(glm::vec3(LEFT_ACCELERATION, g_game_state.player->get_acceleration().y, 0.0f));
        //g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->LEFT];
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        accRight();
        //g_game_state.player->set_acceleration(glm::vec3(RIGHT_ACCELERATION, g_game_state.player->get_acceleration().y, 0.0f));
        //g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];
    }
    else if (key_state[SDL_SCANCODE_UP])
    {
        accUp();
        //g_game_state.player->set_acceleration(glm::vec3(0.0f, g_game_state.player->get_acceleration().y+0.05f, 0.0f));
        //g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->UP];
    }
    else
    {
        // Reset acceleration when no horizontal keys are pressed
        g_game_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY, 0.0f));
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->DOWN];
    }
}

void update()
{
    // ————— DELTA TIME ————— //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    // ————— FIXED TIMESTEP ————— //
    // STEP 1: Keep track of how much time has passed since last step
    delta_time += g_time_accumulator;

    // STEP 2: Accumulate the ammount of time passed while we're under our fixed timestep
    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }

    // STEP 3: Once we exceed our fixed timestep, apply that elapsed time into the objects' update function invocation
    while (delta_time >= FIXED_TIMESTEP)
    {
        // Notice that we're using FIXED_TIMESTEP as our delta time
        g_game_state.gameStatus = g_game_state.player->update(FIXED_TIMESTEP, g_game_state.platforms, PLATFORM_COUNT, g_game_state.failure_platforms, FAILURE_PLATFORMS);
        
        
        delta_time -= FIXED_TIMESTEP;

        if (g_game_state.gameStatus == 1) {
        
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", "You successfully landend! ", g_display_window);
            g_game_is_running = false;
        }

        if (g_game_state.gameStatus == -1) {

            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Failure", "You landed in a crater, you dead. ", g_display_window);
            g_game_is_running = false;
        }

    }

    g_time_accumulator = delta_time;
}

void render()
{
    // ————— GENERAL ————— //
    glClear(GL_COLOR_BUFFER_BIT);

    // ————— PLAYER ————— //
    g_game_state.player->render(&g_shader_program);

    // ————— PLATFORM ————— //
    for (int i = 0; i < PLATFORM_COUNT; i++) g_game_state.platforms[i].render(&g_shader_program);
    for (int i = 0; i < FAILURE_PLATFORMS; i++) g_game_state.failure_platforms[i].render(&g_shader_program);

    // ————— GENERAL ————— //
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

// ————— DRIVER GAME LOOP ————— /
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
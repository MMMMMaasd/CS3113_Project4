/**
* Author: [Michael Bian]
* Assignment: Rise of the AI
* Date due: 2024-11-09, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 1.0f / 60.0f
#define ENEMY_COUNT 3

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //

enum AppStatus  { RUNNING, TERMINATED };

struct GameState
{
    Entity* player;
    Entity* base_platforms;
    Entity* stair_platforms;
    Entity* second_platforms;
    Entity* background;
    Entity* enemies;
    Entity* target;
    Entity* jumpscare;
};

// ––––– CONSTANTS ––––– //
 
constexpr int PLATFORM_COUNT = 45;
constexpr float ACC_OF_GRAVITY = -9.81f;

constexpr int WINDOW_WIDTH  = 640 * 2,
          WINDOW_HEIGHT = 480 * 1.8;

constexpr float LEFT_BORDER = -4.55;
constexpr float RIGHT_BORDER = 4.55;

constexpr float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char SPRITESHEET_FILEPATH[] = "assets/rat.png";
constexpr char PLATFORM_FILEPATH[]    = "assets/platform.png";
constexpr char SECOND_PLATFORM_FILEPATH[]    = "assets/caveFloatingPlatform.png";
constexpr char BACKGROUND_FILEPATH[]    = "assets/horror_background.jpg";
constexpr char MONSTER_FILEPATH[] = "assets/monster365.png";
constexpr char MONSTER_2_FILEPATH[] = "assets/horror_character_2.png";
constexpr char FONT_FILEPATH[] = "assets/font1.png";
constexpr char TARGET_FILEPATH[] = "assets/target.png";
constexpr char JUMP_SCARE_FILEPATH[] = "assets/jump_scare.png";
 
constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

constexpr int CD_QUAL_FREQ    = 44100,
          AUDIO_CHAN_AMT  = 2,     // stereo
          AUDIO_BUFF_SIZE = 4096;

constexpr char BGM_FILEPATH[] = "assets/Jigoku Shoujo OST 1 - 17.Jigoku Nagashi.mp3",
           SFX_FILEPATH[] = "assets/scary scream jumpscare sound effect.wav",
SCREAM_FILEPATH[] = "assets/scary scream jumpscare sound effect.mp3";


constexpr int PLAY_ONCE = 0,    // play once, loop never
          NEXT_CHNL = -1,   // next available channel
          ALL_SFX_CHNL = -1;

 
Mix_Music *g_music;
Mix_Music *g_scream;
Mix_Chunk *g_jump_sfx;

constexpr glm::vec3 SUBMARINE_INITSCALE = glm::vec3(1.37f, 1.0f, 0.0f);


// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

AppStatus g_app_status = RUNNING;

SDL_Window* g_display_window;

bool ifGameEnd = false;
bool ifWin = false;
bool ifScreamed = false;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;
int jump_scare_counter = 0;

GLuint g_font_texture_id;
// ––––– GENERAL FUNCTIONS ––––– //
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
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Hello, Physics (again)!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ––––– BGM ––––– //
    Mix_OpenAudio(CD_QUAL_FREQ, MIX_DEFAULT_FORMAT, AUDIO_CHAN_AMT, AUDIO_BUFF_SIZE);

    // STEP 1: Have openGL generate a pointer to your music file
    g_music = Mix_LoadMUS(BGM_FILEPATH); // works only with mp3 files
    g_scream = Mix_LoadMUS(SCREAM_FILEPATH);

    // STEP 2: Play music
    Mix_PlayMusic(
                  g_music,  // music file
                  -1        // -1 means loop forever; 0 means play once, look never
                  );
    
    // STEP 3: Set initial volume
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2.0);

    // ––––– SFX ––––– //
    g_jump_sfx = Mix_LoadWAV(SFX_FILEPATH);

    // ––––– PLATFORMS ––––– //
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
    GLuint second_platform_texture_id = load_texture(SECOND_PLATFORM_FILEPATH);
    GLuint background_texture_id = load_texture(BACKGROUND_FILEPATH);
    g_font_texture_id = load_texture(FONT_FILEPATH);

    g_state.background = new Entity();
    g_state.background->set_texture_id(background_texture_id);
    g_state.background->set_scale(glm::vec3(13.26, 7.6, 0.0f));
    g_state.background->update(0.0f, NULL, NULL, 0);
    
    g_state.base_platforms = new Entity[PLATFORM_COUNT];
    
    int flip_counter = 0;
    // Set the type of every platform entity to PLATFORM
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        if(i <= 23){
            g_state.base_platforms[i].set_texture_id(platform_texture_id);
            g_state.base_platforms[i].set_position(glm::vec3(((i - PLATFORM_COUNT / 1.8f)+7.0f), -3.5f, 0.0f));
            g_state.base_platforms[i].set_width(1.35f);
            g_state.base_platforms[i].set_height(1.35f);
            g_state.base_platforms[i].set_scale(glm::vec3(1.0f, 1.0f, 0.0f));
            g_state.base_platforms[i].set_entity_type(PLATFORM);
            g_state.base_platforms[i].update(0.0f, NULL, NULL, 0);
        } else if(i <= 29){
            g_state.base_platforms[i].set_texture_id(second_platform_texture_id);
            g_state.base_platforms[i].set_position(glm::vec3(((i-24 - 10 / 1.5f)+1.3), -1.15f, 0.0f));
            g_state.base_platforms[i].set_width(2.7f);
            g_state.base_platforms[i].set_height(0.35f);
            g_state.base_platforms[i].set_scale(glm::vec3(3.16f, 0.5f, 0.0f));
            g_state.base_platforms[i].set_entity_type(PLATFORM);
            if(!flip_counter){
                g_state.base_platforms[i].set_rotate_angle(180);
            }
            g_state.base_platforms[i].update(0.0f, NULL, NULL, 0);
            flip_counter = !flip_counter;
        } else if(i < 31){
            g_state.base_platforms[i].set_texture_id(second_platform_texture_id);
            g_state.base_platforms[i].set_position(glm::vec3(-((0 - 23 / 1.5f)+11.7), -2.05f, 0.0f));
            g_state.base_platforms[i].set_width(3.1f);
            g_state.base_platforms[i].set_height(0.35f);
            g_state.base_platforms[i].set_scale(glm::vec3(3.16f, 0.5f, 0.0f));
            g_state.base_platforms[i].set_entity_type(PLATFORM);
            g_state.base_platforms[i].update(0.0f, NULL, NULL, 0);
        } else if(i < 41){
            g_state.base_platforms[i].set_texture_id(second_platform_texture_id);
            g_state.base_platforms[i].set_position(glm::vec3(-((i-31 - 10 / 1.5f)-1.5f), -0.1f, 0.0f));
            g_state.base_platforms[i].set_width(2.5f);
            g_state.base_platforms[i].set_height(0.36f);
            g_state.base_platforms[i].set_scale(glm::vec3(3.16f, 0.5f, 0.0f));
            g_state.base_platforms[i].set_entity_type(PLATFORM);
            g_state.base_platforms[i].update(0.0f, NULL, NULL, 0);
        } else if(i < 42){
            g_state.base_platforms[i].set_texture_id(second_platform_texture_id);
            g_state.base_platforms[i].set_position(glm::vec3(-((i-31 - 10 / 1.5f) + 1.7f), -0.5f, 0.0f));
            g_state.base_platforms[i].set_width(2.6f);
            g_state.base_platforms[i].set_height(0.36f);
            g_state.base_platforms[i].set_scale(glm::vec3(3.16f, 0.5f, 0.0f));
            g_state.base_platforms[i].set_entity_type(PLATFORM);
            g_state.base_platforms[i].update(0.0f, NULL, NULL, 0);
        } else if(i < 43){
            g_state.base_platforms[i].set_texture_id(second_platform_texture_id);
            g_state.base_platforms[i].set_position(glm::vec3(-((i-31 - 10 / 1.5f) - 9.0f), 1.2f, 0.0f));
            g_state.base_platforms[i].set_width(1.2f);
            g_state.base_platforms[i].set_height(0.09f);
            g_state.base_platforms[i].set_scale(glm::vec3(1.58f, 0.25f, 0.0f));
            g_state.base_platforms[i].set_entity_type(PLATFORM);
            g_state.base_platforms[i].update(0.0f, NULL, NULL, 0);
        } else if(i < 44){
            g_state.base_platforms[i].set_texture_id(second_platform_texture_id);
            g_state.base_platforms[i].set_position(glm::vec3(-((i-31 - 10 / 1.5f) - 8.8f), 0.9f, 0.0f));
            g_state.base_platforms[i].set_width(1.2f);
            g_state.base_platforms[i].set_height(0.09f);
            g_state.base_platforms[i].set_scale(glm::vec3(1.30f, 0.25f, 0.0f));
            g_state.base_platforms[i].set_entity_type(PLATFORM);
            g_state.base_platforms[i].update(0.0f, NULL, NULL, 0);
        } else{
            g_state.base_platforms[i].set_texture_id(second_platform_texture_id);
            g_state.base_platforms[i].set_position(glm::vec3(-((i-31 - 10 / 1.5f) - 8.6f), 0.6f, 0.0f));
            g_state.base_platforms[i].set_width(1.2f);
            g_state.base_platforms[i].set_height(0.09f);
            g_state.base_platforms[i].set_scale(glm::vec3(1.30f, 0.25f, 0.0f));
            g_state.base_platforms[i].set_entity_type(PLATFORM);
            g_state.base_platforms[i].update(0.0f, NULL, NULL, 0);
        }
    }

    


    // ––––– PLAYER (GEORGE) ––––– //
    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);

    int player_walking_animation[4][3] =
    {
        { 0, 1, 2 },  // for player to move to the right,
        { 3, 4, 5 }, // for player to move to the left,
        { 6, 7, 8 }, // for player to move downwards,
        { 9, 10, 11 }   // for player to move upwards
    };

    glm::vec3 acceleration = glm::vec3(0.0f, -9.8f, 0.0f);

    g_state.player = new Entity(
        player_texture_id,         // texture id
        3.0f,                      // speed
        acceleration,              // acceleration
        4.0f,                      // jumping power
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        3,                         // animation frame amount
        0,                         // current animation index
        3,                         // animation column amount
        4,                         // animation row amount
        0.22f,                      // width
        0.44f,                       // height
        PLAYER
    );

    //g_state.player->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_state.player->set_scale(glm::vec3(0.8f, 0.8f, 0.8f));
    g_state.player->set_position(glm::vec3(-4.0f, -2.0f, 0.0f));
    // Jumping
    g_state.player->set_jumping_power(4.5f);
    
    // AI Enemies
    GLuint enemy_texture_id = load_texture(MONSTER_FILEPATH);
    GLuint enemy_2_texture_id = load_texture(MONSTER_2_FILEPATH);
    g_state.enemies = new Entity[ENEMY_COUNT];
    
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        if(i == 0){
            g_state.enemies[i] =  Entity(enemy_texture_id, 1.0f, 0.7f, 0.7f, ENEMY, GUARD, IDLE);
        } else if(i == 1){
            g_state.enemies[i] =  Entity(enemy_2_texture_id, -1.0f, 0.5f, 0.7f, ENEMY, PATROLLING, RIGHTMOVING);
        } else {
            g_state.enemies[i] =  Entity(enemy_texture_id, 1.0f, 0.7f, 0.9f, ENEMY, JUMPER, IDLE);
        }
    }
    
    
    g_state.enemies[0].set_position(glm::vec3(1.7f, -4.5f, 0.0f));
    g_state.enemies[0].set_movement(glm::vec3(0.0f));
    g_state.enemies[0].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    g_state.enemies[0].set_entity_type(ENEMY);
    
    g_state.enemies[1].set_position(glm::vec3(-1.3f, 0.5f, 0.0f));
    g_state.enemies[1].set_movement(glm::vec3(0.0f));
    g_state.enemies[1].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    g_state.enemies[1].set_entity_type(ENEMY);
    g_state.enemies[1].set_scale(glm::vec3(1.46f, 1.2f, 0.0f));
    
    g_state.enemies[2].set_position(glm::vec3(0.2f, 1.8f, 0.0f));
    g_state.enemies[2].set_movement(glm::vec3(0.0f));
    g_state.enemies[2].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    g_state.enemies[2].set_entity_type(ENEMY);

    GLuint target_texture_id = load_texture(TARGET_FILEPATH);
    g_state.target = new Entity();
    g_state.target->set_texture_id(target_texture_id);
    g_state.target->set_position(glm::vec3(4.5f, 1.57f, 0.0f));
    g_state.target->set_scale(glm::vec3(0.8f, 0.8f, 0.0f));
    g_state.target->update(0.0f, NULL, NULL, 0);
    
    GLuint jump_scare_texture_id = load_texture(JUMP_SCARE_FILEPATH);
    g_state.jumpscare = new Entity();
    g_state.jumpscare->set_texture_id(jump_scare_texture_id);
    g_state.jumpscare->set_scale(glm::vec3(8.0, 8.0, 0.0f));
    g_state.jumpscare->update(0.0f, NULL, NULL, 0);
    
    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_app_status = TERMINATED;
                        break;

                    case SDLK_SPACE:
                        // Jump
                        if((g_state.player->get_isHide())){
                            return;
                        }
                        if (g_state.player->get_collided_bottom())
                        {
                            g_state.player->jump();
                            // Mix_PlayChannel(NEXT_CHNL, g_jump_sfx, 0);
                        }
                        break;

                    case SDLK_h:
                        // Stop music
                        Mix_HaltMusic();
                        break;

                    case SDLK_p:
                        Mix_PlayMusic(g_music, -1);

                    default:
                        break;
                }

            default:
                break;
        }
    }
    
    g_state.player->set_un_hiding();

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        if(!ifGameEnd){
            g_state.player->move_left();
        }
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        if(!ifGameEnd){
            g_state.player->move_right();
        }
    }else if (key_state[SDL_SCANCODE_DOWN]){
        if(!ifGameEnd){
            g_state.player->set_hiding();
        }


    }

    if (glm::length(g_state.player->get_movement()) > 1.0f)
    {
        g_state.player->normalise_movement();
    }
}

void update()
{
    if(ifGameEnd && !ifWin){
        ++jump_scare_counter;
    }
    if(ifGameEnd){
        return;
    }
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        g_state.player->update(FIXED_TIMESTEP, NULL, g_state.base_platforms, g_state.enemies, PLATFORM_COUNT, ENEMY_COUNT);
        for (int i = 0; i < ENEMY_COUNT; i++)
            g_state.enemies[i].update(FIXED_TIMESTEP,
                                      g_state.player,
                                      g_state.base_platforms,
                                                   PLATFORM_COUNT);
        if(g_state.player->get_collided_enemy()){
            ifGameEnd = true;
        }
        
        if((g_state.player->get_position()).x >= 4.5f && (g_state.player->get_position()).y >= 1.57f){
            ifGameEnd = true;
            ifWin = true;
        }
        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
}

constexpr int FONTBANK_SIZE = 16;

void draw_text(ShaderProgram *program, GLuint font_texture_id, std::string text,
               float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0,
                          vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
                          texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_state.background->render(&g_shader_program);
    
    g_state.player->render(&g_shader_program);

    for (int i = 0; i < PLATFORM_COUNT; ++i) g_state.base_platforms[i].render(&g_shader_program);
    for (int i = 0; i < ENEMY_COUNT; i++) g_state.enemies[i].render(&g_shader_program);
    
    if(ifGameEnd && !ifWin){
        draw_text(&g_shader_program, g_font_texture_id, "**You Lose**", 0.5f, 0.05f,
                      glm::vec3(-3.1f, 3.0f, 0.0f));
    }
    
    if(ifGameEnd && ifWin){
        draw_text(&g_shader_program, g_font_texture_id, "You Win!", 0.5f, 0.05f,
                      glm::vec3(-3.5f, 2.0f, 0.0f));
    }
    
    g_state.target->render(&g_shader_program);
    
    draw_text(&g_shader_program, g_font_texture_id, "left-move left  right-move right", 0.23f, 0.0f, glm::vec3(-4.8f, -3.3f, 0.0f));
    draw_text(&g_shader_program, g_font_texture_id, "space-jump  down-hide(hide you from attack)", 0.23f, -0.0f, glm::vec3(-4.8f, -3.6f, 0.0f));

    if(jump_scare_counter>=290 && !ifScreamed){
        ifScreamed = true;
        Mix_PlayChannel(NEXT_CHNL, g_jump_sfx, 0);
    }
    if(jump_scare_counter>=300){
        g_state.jumpscare->render(&g_shader_program);
    }
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete [] g_state.base_platforms;
    delete [] g_state.second_platforms;
    delete [] g_state.stair_platforms;
    delete [] g_state.enemies;
    delete g_state.player;
    delete g_state.background;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}

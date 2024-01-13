#ifndef SPIROGRAPH_H
#define SPIROGRAPH_H

// * HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <chrono>
#include <math.h>
#include <SDL.h>

// * MACRODEFINITIONS
// Constants
#define PI 3.14159265358979323

// Colours
#define RGBA_EXPAND(colour) colour.r, colour.g, colour.b, colour.a
#define WHITE 255, 255, 255, 255
#define BLACK 0, 0, 0, 255
#define RED 255, 0, 0, 255
#define GREEN 0, 255, 0, 255
#define BLUE 0, 0, 255, 255
#define YELLOW 255, 255, 0, 255
#define ORANGE 255, 100, 0, 255
#define PURPLE 255, 0, 255, 255

// Default settings
#define DEFAULT_LENGTH 100
#define DEFAULT_ANGLE 0
#define DEFAULT_REVPS 1

// * TYPE DEFINITIONS
template <typename T>
struct Vec2
{
    T x, y;
    float length();
};
typedef Vec2<int> Vec2Int;
typedef Vec2<float> Vec2Float;
typedef Vec2<double> Vec2Double;

typedef struct
{
    float r, g, b, a;
} RGBA;

typedef struct
{
    float h, s, v, a;
} HSVA;

bool play = true;
enum Mode {EDIT, ANIMATE};

// * CLASS PROTOTYPES
class Trail
{
    public:
        Vec2Float first_point;
        Vec2Float current_point;
        Vec2Float previous_point;
        RGBA colour;
        int length;

        Trail(RGBA rgba);
        void draw();
        void new_point(Vec2Float point0);
        void reset();
};

class Spirograph
{
    public:
        // * Members
        Vec2Float position_initial, position;
        Vec2Float direction_initial, direction;

        float revps;
        bool is_root;
        const SDL_Scancode delete_node_key = SDL_SCANCODE_BACKSPACE;
        const SDL_Scancode reset_key = SDL_SCANCODE_R;

        // Parent members
        Spirograph *parent;
        float position_on_parent; // Value in the range [0, 1] describing the location of the node on its parent node
        
        Spirograph **children;
        int children_length;

        // Trail members
        bool trail_on;
        Trail *trail;
        const SDL_Scancode toggle_trail_key = SDL_SCANCODE_Q;

        // Head and base
        int head_radius, base_radius;
        const int default_radius = 5;
        const int hover_radius = 7;
        const SDL_Scancode change_head_key = SDL_SCANCODE_E;
        const SDL_Scancode change_base_key = SDL_SCANCODE_W;
        
        // Highlight colours
        enum HighlightType {UNHIGHLIGHT = 0, LIGHT_HIGHLIGHT = 1, HIGHLIGHT = 2};
        const float highlightAlpha[3] = {255*0.25f, 255*0.5f, 255.0};
        const RGBA highlightColour[3] = {{50, 50, 50, 255}, {150, 150, 150, 255}, {255, 255, 255, 255}};


        // * Methods 
        Spirograph(Vec2Float position_i, Vec2Float direction_i);
        void rotate(double dt);
        void reset();
        void update_childrens_position_on_parent();

        void draw_trail();
        void update_trail_first_point();
        void draw(enum HighlightType);
        void draw_direction(enum HighlightType);
        void draw_head(enum HighlightType);
        void draw_base(enum HighlightType);

        void add_child(Spirograph *new_child_ptr);
        void remove_child(Spirograph *old_child_ptr);
        void clear_children();
        
        void free_members();
        Vec2Float get_cursor_orthogonalProjection();
};

// * GLOBAL VARIABLES
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *trail_texture;
SDL_Event event;

struct
{
    int width, height;
    RGBA background_colour;
} display;

struct
{
    Vec2Int pos, left_down_pos, left_up_pos, right_down_pos, right_up_pos;
    bool left_down, right_down, left_up, right_up, scroll_up, scroll_down;
} MouseState;

struct KeyboardState
{
    const Uint8 *keystates;
    bool keydown(SDL_Scancode keycode);
    bool keyup(SDL_Scancode keycode);
} keyboardState;

struct EditorState {
    bool creating_first;

    enum {
        SET_CHILD_POSITION,
        SET_CHILD_DIRECTION,
        EDIT_MENU,
        PREVIEW
    } edit_mode;

    EditorState() :
        creating_first(true),
        edit_mode(SET_CHILD_POSITION)
    {}
};
EditorState editorState;

// * FUNCTION PROTOTYPES
void edit(Spirograph *rootNode, double dt);
void edit_dirpos(Spirograph *selected_node, bool *editing);
void reselect_node(Spirograph *rootNode, Spirograph *closest_node, Spirograph **selected_node, bool editing_dirpos);
void node_near_cursor_orthproj(Spirograph *root, Spirograph **closest, float *distance2, Vec2Float *orthproj);
void colour_palette(Spirograph *current_node, bool *hovering);
void change_rotation_speed(Spirograph *selected_node, bool *editing, double dt);

// Colour functions
RGBA hsva_to_rgba(HSVA in);
HSVA rgba_to_hsva(RGBA in);

// SDL Functions
void initialize_SDL();
void quit_SDL();
void handleEvents(bool *running, enum Mode *mode, Spirograph *root);
void clearRenderer();

// SDL Draw Functions
int SDL_RenderDrawCircle(SDL_Renderer * renderer, int x, int y, int radius);
int SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y, int radius);
void drawLine(SDL_Renderer *renderer, RGBA colour, int x0, int y0, int x1, int y1);

#endif
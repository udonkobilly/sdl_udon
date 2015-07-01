#ifndef __SDL_UDON_PRIVATE_H__
#define __SDL_UDON_PRIVATE_H__

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_mixer.h"
#include "SDL2/SDL_ttf.h"
#include "ruby.h"

#include "ruby/thread.h"

#define BASE_WINDOW_WIDTH 640
#define BASE_WINDOW_HEIGHT 480
#define BASE_FPS 60

#define SCANCODE_MAX (283)
#define MATH_PI (3.141592653589793)
#define MATH_DOUBLE_PI (MATH_PI*2)

#define DRAW_TEXT_CACHE 1024*1024*30

typedef struct {
    st_table* table;
    BOOL complete;
    BOOL error;
} LoaderData;

typedef struct {
    Uint32 interval;
    int loop;
    int loop_count;
    VALUE proc;
    VALUE counter;
    BOOL started, paused;
} CronData;

typedef struct {
    VALUE startCount, nowCount;
    VALUE max, min;
    int step;
} CounterData;

typedef struct {
    SDL_Texture* texture;
    SDL_Color colorKey;
    SDL_Rect *rect;
    uint8_t attach_id;
} ImageData;

typedef enum {
    COLLISION_AREA_NONE = 0,
    COLLISION_AREA_POINT = 1,
    COLLISION_AREA_LINE = 2,
    COLLISION_AREA_TRIANGLE = 4,
    COLLISION_AREA_RECT = 8,
    COLLISION_AREA_CIRCLE = 16,
} CollisionAreaType; 

struct _collisionArea;

typedef struct _collisionArea {
    CollisionAreaType type;
    int pointLen;
    double *points; 
    char *key;
    BOOL active; 
    struct _collisionArea* next;
    struct _collisionArea* prev;
} CollisionArea;

typedef struct {
    double crossProduct;
    double intersectX, intersectY;
} HitData;

struct _hitObjectData;
typedef struct _hitObjectData {
    struct _hitObjectData *next;
    CollisionArea* targetArea;
    CollisionArea* selfArea;
    int selfIndex, targetIndex;
} HitObjectData;

typedef struct {
    VALUE x,y;
    BOOL active, visible, collidable, suspend;
} ActorData;

struct _drawTextCache;
typedef struct _drawTextCache {
    char* text;
    TTF_Font *font;
    int size;
    SDL_Color color; 
    SDL_Texture *texture;
    int textureByteSize;
    struct _drawTextCache *prev;
    struct _drawTextCache *next;
} DrawTextCache;

typedef struct {
    SDL_Point *pt;
    BOOL active, suspend;
    BOOL collidable, childPoolLock;    
    int fps;
    float realFps;
    Uint32 windowID;
    SDL_Window *window;
    SDL_Renderer *renderer;
    BOOL visible, running;
    BOOL mouseFocus,keyboardFocus; 
    BOOL checkRenderable;
} GameData;
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    double_t fps;
    uint64_t totalFrame;
    BOOL rootGameCreated, gameRunning;
    BOOL active;
} SystemData;

struct _multiGameApplication;
typedef struct _multiGameApplication {
    GameData *data;
    struct _multiGameApplication *next;
    VALUE obj;    
} MultiGameApplication;

typedef struct {
    VALUE fiber;
    VALUE run_method;
    VALUE reserve_state_name;
    BOOL suspend;
    BOOL changed;
    int delay;
} StateMachineData;

typedef struct {
    SDL_Texture* texture;
    SDL_Renderer* renderer;
    SDL_Rect *rect;
    DrawTextCache* drawTextCache;
    uint32_t drawTextCacheSize;
    uint8_t attach_id;
} ScreenData;

typedef struct {
    int duration, wait, interval;
    int durationCount, waitCount, intervalCount;
} RepeatData;

typedef struct {
    RepeatData *repeatData;
    Uint8 state; 
} KeyData;

typedef struct {
    KeyData keyData[SDL_SCANCODE_KP_PERIOD - SDL_SCANCODE_A + 1 + 8];
} KeyBoardData;

typedef struct {
    int x,y, dx,dy;
    int wheel;
    Uint8 btn_states[3];
    RepeatData *btnRepeatDatas[3];
    BOOL moving;
} MouseData;

typedef struct {
    KeyBoardData *key;
    MouseData *mouse;
} InputData;

typedef struct {
    TTF_Font *font;
    char* name;
} FontData;

struct _fontCache;
typedef struct _fontCache {
    TTF_Font *font;
    int size;
    char* name;
    struct _fontCache *next;
} FontCache;

typedef struct {
    double_t nowTime; 
    double_t duration;
    double_t (*easing)();
    BOOL active; 
} TweenData;

typedef struct {
    BOOL looped;
} TimeLineData;

typedef struct {
} EventManagerData;



typedef struct {
    float volume; 
    Mix_Music *music;
} MusicData;

typedef struct {
    Mix_Chunk *chunk;
    float volume;
    int channel;
} EffectData;


struct _soundMusicCache;
typedef struct _soundMusicCache {
    char *name;
    Mix_Music *music;
    struct _soundMusicCache *next;
} SoundMusicCache;

struct _soundEffectCache;
typedef struct _soundEffectCache {
    char *name;
    Mix_Chunk *chunk;
    struct _soundEffectCache *next;
} SoundEffectCache;

Mix_Music* CreateBGMFromMEMAtCache(const char* fileName, const Uint8*buff);
Mix_Chunk* CreateSEFromMEMAtCache(const char* fileName, const Uint8*buff);

extern SystemData *systemData; 
extern InputData *inputData; 

ID id_iv_scene;
ID id_iv_screen;
ID id_iv_state_machine;
ID id_iv_pool;
ID id_each,id_combination;
ID id_update, id_collied, id_draw, id_draw_actor, id_run;
ID id_is_active, id_is_halt, id_is_visible;
ID id_x, id_y;
ID id_trigger;
ID id_iv_event_manager;
ID id_iv_timeline;
ID id_hit;
ID id_delete_if;
ID id_draw_ex, id_draw_opts, id_iv_image;

#define P( msg ) fprintf(stderr, "%s,%d:%s\n",__FILE__,__LINE__,msg)
#define SDL_P() fprintf(stderr, "%s,%d:%s\n",__FILE__,__LINE__, (SDL_GetError()))
#define LOGFILENAME "log.txt"
#define LOG(message)

void p(const char *format, ...);
void logger(const char* filename, const char* message, const int line, const char* srcfile);
FILE* safeFOPEN(const char *filename, const char *mode);

void SDLErrorAbortBody(BOOL loggable, const char *message);
#define SDL_ABORT() SDLErrorAbortBody(FALSE, "")
#define SDL_LOG_ABORT() SDLErrorAbortBody(TRUE, "")
#define SDL_DETAILLOG_ABORT(message) SDLErrorAbortBody(TRUE, message)


SDL_Renderer* GetWindowRenderer(VALUE *obj, Uint32 window_id);

void renderTextureRect(SDL_Texture *tex, SDL_Renderer *ren, int x, int y, int x2, int y2, int w, int h);
void renderTexture(SDL_Texture *tex, SDL_Renderer *ren, SDL_Rect *dst);
SDL_Texture* renderText(const char* str, const char* fontName, SDL_Color color, int  fontSize, SDL_Renderer *ren);
void SetWindowCaption(const char* title);
SDL_Surface* TextureToSurface(SDL_Texture* texture, Uint32 * colorKey);
SDL_Texture* CreateTextureFromSurface(SDL_Surface* surface, SDL_Renderer* ren, SDL_Color *outColorKey);

void FreeFontCache();
void FreeSoundCache();
void SetUpSound();
ImageData* CreateCImage(int width, int height, SDL_Renderer* ren, ImageData* image);
void FreeCImage(ImageData *sprite);
void Init_manager(VALUE parent_module);
void Init_counter(VALUE parent_module);
void Init_screen(VALUE parent_module);
void Init_image(VALUE parent_module);
void Init_collision(VALUE parent_module);
void Init_actor(VALUE parent_module);
void Init_font(VALUE parent_module);
void Init_scene(VALUE parent_module);
void Init_statemachine(VALUE parent_module);

void InputManager(); 
void InputRefresh();
void GetMouseWheelY(Sint32 *y);
void SetUpSDLGameController();
void joyPadButtonUpManager(SDL_JoyButtonEvent *event);
void joyPadAxisMotionManager(SDL_JoyAxisEvent *event);
void Init_input(VALUE parent_module);
void Quit_input();
void Init_sound(VALUE parent_module);

void Init_timeline();
void Init_tween();
void Init_event();
void Update_event();
void Quit_event();
void Quit_font();

#endif
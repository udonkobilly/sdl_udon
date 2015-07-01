#include "sdl_udon_private.h"

static VALUE game_is_fullscreen(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    return (SDL_GetWindowFlags(gameData->window) & SDL_WINDOW_FULLSCREEN) ? Qtrue : Qfalse;
}

static VALUE game_set_fullscreen(VALUE self, VALUE true_or_false) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    if (true_or_false == Qtrue) {
        SDL_SetRenderTarget(gameData->renderer, NULL);
        SDL_SetWindowFullscreen(gameData->window, SDL_WINDOW_FULLSCREEN);
    } else {
        SDL_SetWindowFullscreen(gameData->window, 0);        
    }
    return self;
}

static VALUE game_set_fps(VALUE self, VALUE fps) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    gameData->fps = DBL2NUM(fps);
    return Qnil;
}

static VALUE game_get_fps(VALUE self) { 
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    return rb_float_new(gameData->fps); 
}

static VALUE game_set_x(VALUE self, VALUE x) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    int old_x, y; SDL_GetWindowPosition(gameData->window, &old_x, &y);
    int new_x = FIX2INT(x); SDL_SetWindowPosition(gameData->window, new_x, y);
    return Qnil;
}

static VALUE game_get_x(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    int x, y; SDL_GetWindowPosition(gameData->window, &x, &y);
    return rb_int_new(x);
}
static VALUE game_set_y(VALUE self, VALUE y) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    int x, old_y; SDL_GetWindowPosition(gameData->window, &x, &old_y);
    int new_y = FIX2INT(y); SDL_SetWindowPosition(gameData->window, x, new_y);
    return Qnil;
}

static VALUE game_get_y(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    int x, y; SDL_GetWindowPosition(gameData->window, &x, &y);
    return rb_int_new(y);
}

static VALUE game_get_x2(VALUE self) {
    VALUE x = rb_funcall(self, rb_intern("x"), 0);
    VALUE width = rb_funcall(self, rb_intern("width"), 0);
    return rb_funcall(x, rb_intern("+"), 1, width);
}

static VALUE game_get_y2(VALUE self) {
    VALUE y = rb_funcall(self, rb_intern("y"), 0);
    VALUE height = rb_funcall(self, rb_intern("height"), 0);
    return rb_funcall(y, rb_intern("+"), 1, height);
}

static VALUE game_get_width(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    int w, h; 
    SDL_GetWindowSize(gameData->window, &w, &h);

    return rb_int_new(w);
}

static VALUE game_set_width(VALUE self, VALUE width) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    int w, h; SDL_GetWindowSize(gameData->window, &w, &h);
    int new_width = FIX2INT(width); SDL_SetWindowSize(gameData->window, new_width, h);
    BOOL isSystemRenderer = FALSE;
    if (systemData->renderer == gameData->renderer) isSystemRenderer = TRUE; 
    SDL_DestroyRenderer(gameData->renderer);
    gameData->renderer = SDL_CreateRenderer(gameData->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (isSystemRenderer) systemData->renderer = gameData->renderer;

    return Qnil;
}

static VALUE game_get_height(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    int w, h; SDL_GetWindowSize(gameData->window, &w, &h);
    return rb_int_new(h);
}

static VALUE game_set_height(VALUE self, VALUE height) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    int w, h; SDL_GetWindowSize(gameData->window, &w, &h);
    int new_height = FIX2INT(height); SDL_SetWindowSize(gameData->window, w, new_height);
    BOOL isSystemRenderer = FALSE;
    if (systemData->renderer == gameData->renderer) isSystemRenderer = TRUE; 
    SDL_DestroyRenderer(gameData->renderer);
    gameData->renderer = SDL_CreateRenderer(gameData->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (isSystemRenderer) systemData->renderer = gameData->renderer;

    return Qnil;
}

static VALUE game_set_caption(volatile VALUE self, volatile VALUE title) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    SDL_SetWindowTitle(gameData->window, StringValuePtr(title));
    return Qnil;
}

static VALUE game_get_caption(VALUE self) { 
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    return rb_str_new2(SDL_GetWindowTitle(gameData->window)); 
}

static VALUE game_focus(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    if (!gameData->visible) SDL_ShowWindow(gameData->window);
    SDL_RaiseWindow(gameData->window);
    return self;
}

static VALUE game_is_focus(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    return (gameData->keyboardFocus) ? Qtrue : Qfalse;
}

static VALUE game_is_mouse_hover(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    return (gameData->mouseFocus) ? Qtrue : Qfalse;
}

static VALUE game_is_visible(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    return (gameData->visible) ? Qtrue : Qfalse;
}

static VALUE game_show(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    SDL_ShowWindow(gameData->window);
    gameData->visible = TRUE;
    return self;
}

static VALUE game_hide(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    SDL_HideWindow(gameData->window);
    gameData->visible = FALSE;
    return self;
}

static VALUE game_is_active(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    return gameData->active ? Qtrue : Qfalse;    
}

static VALUE game_get_real_fps(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    return rb_float_new(gameData->realFps);
}

static VALUE game_alloc(VALUE klass) {
    GameData *game_info = ALLOC(GameData);
    return Data_Wrap_Struct(klass, 0, -1, game_info);
}
static VALUE game_attach(VALUE self, VALUE attach_obj) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    if ( rb_obj_class(attach_obj) == rb_path2class("SDLUdon::Screen") ) {
        if (rb_equal(rb_ivar_get(self, rb_intern("@render_target")), attach_obj) == Qtrue) return Qnil;
    } else if ( rb_obj_class(attach_obj) == rb_path2class("SDLUdon::Actor") ) {
        VALUE image = rb_ivar_get(attach_obj, rb_intern("@image"));
        ImageData *imageData; Data_Get_Struct(image, ImageData, imageData);
        if (gameData->windowID != imageData->attach_id) {
            SDL_PixelFormat *temp_fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
            Uint32 colorCode = SDL_MapRGBA(temp_fmt, imageData->colorKey.r,imageData->colorKey.g,imageData->colorKey.b,imageData->colorKey.a);
            SDL_FreeFormat(temp_fmt);
            SDL_Surface* surface = TextureToSurface(imageData->texture, &colorCode);
            SDL_DestroyTexture(imageData->texture);
            imageData->texture = CreateTextureFromSurface(surface, gameData->renderer, &imageData->colorKey);
            SDL_FreeSurface(surface);
            imageData->attach_id = gameData->windowID;
        }
    }
    return attach_obj;
}

static VALUE game_is_attached(VALUE self, VALUE attach_obj) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    if ( rb_obj_class(attach_obj) == rb_path2class("SDLUdon::Screen") ) {
        ScreenData* screen; Data_Get_Struct(attach_obj, ScreenData, screen);
        return (gameData->windowID == screen->attach_id) ? Qtrue : Qfalse;
    } else if ( rb_obj_class(attach_obj) == rb_path2class("SDLUdon::Actor") ) {
        VALUE image = rb_ivar_get(attach_obj, rb_intern("@image"));
        ImageData *imageData; Data_Get_Struct(image, ImageData, imageData);
        return (gameData->windowID == imageData->attach_id) ? Qtrue : Qfalse;
    } else {
        return Qnil;
    }
}

static void game_init_option_parse(volatile VALUE game, volatile VALUE hash) {
    volatile VALUE v = rb_hash_lookup(hash, ID2SYM(rb_intern("title")));
    if (!NIL_P(v)) game_set_caption(game, v);
    v = rb_hash_lookup(hash, ID2SYM(rb_intern("fps")));
    if (!NIL_P(v)) game_set_fps(game, v);
    v = rb_hash_lookup(hash, ID2SYM(rb_intern("x")));
    if (!NIL_P(v)) game_set_x(game, v);
    v = rb_hash_lookup(hash, ID2SYM(rb_intern("y")));
    if (!NIL_P(v)) game_set_y(game, v);
    v = rb_hash_lookup(hash, ID2SYM(rb_intern("width")));
    if (!NIL_P(v)) game_set_width(game, v);
    v = rb_hash_lookup(hash, ID2SYM(rb_intern("height")));
    if (!NIL_P(v)) game_set_height(game, v);

    v = rb_hash_lookup(hash, ID2SYM(rb_intern("fullscreen")));
    if (!NIL_P(v)) game_set_fullscreen(game, v);

    v = rb_hash_lookup(hash, ID2SYM(rb_intern("store")));
    if (!NIL_P(v)) rb_funcall(rb_path2class("SDLUdon::Game"), rb_intern("store"), 1, v);

}

static void CreateGameWindowAndRenderer(SDL_Window** window, SDL_Renderer** renderer) {
   if (systemData->rootGameCreated) {
        *window = SDL_CreateWindow("no title.", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        BASE_WINDOW_WIDTH, BASE_WINDOW_HEIGHT, SDL_WINDOW_HIDDEN); 
        if (*window == NULL) SDL_LOG_ABORT();
        *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
        if (*renderer == NULL) {
            SDL_DestroyWindow(*window);
            SDL_LOG_ABORT();
        }
    } else {
        *window = systemData->window;
        *renderer = systemData->renderer;
        systemData->rootGameCreated = TRUE;
    }
}

static VALUE game_initialize(int argc, VALUE argv[], VALUE self) {
    GameData *gameData; Data_Get_Struct(self, GameData, gameData);
    CreateGameWindowAndRenderer(&gameData->window, &gameData->renderer);
    volatile VALUE hash;
    if (rb_scan_args(argc, argv, "01", &hash) == 1) game_init_option_parse(self, hash);


    VALUE screen_argv[] = { game_get_width(self), game_get_height(self) };
    volatile VALUE screen = rb_class_new_instance(2, screen_argv, rb_path2class("SDLUdon::Screen"));
    rb_ivar_set(self, rb_intern("@mouse_hover"), Qfalse);
    rb_ivar_set(self, rb_intern("@image"), Qnil);

    rb_ivar_set(self, rb_intern("@child_lock"), Qfalse);
    rb_ivar_set(self, rb_intern("@screen"), screen);
    rb_ivar_set(self, rb_intern("@pool"), rb_ary_new()); 
    rb_ivar_set(self, id_iv_state_machine, rb_class_new_instance(0, NULL, rb_path2class("SDLUdon::StateMachine")));
    rb_ivar_set(self, rb_intern("@event_manager"), rb_class_new_instance(0, NULL, rb_path2class("SDLUdon::Event::Manager")));
    rb_ivar_set(self, rb_intern("@timeline"), rb_class_new_instance(1, (VALUE[]) { self }, rb_path2class("SDLUdon::Timeline")));

    gameData->suspend = FALSE;
    gameData->fps = BASE_FPS;
    gameData->active = gameData->visible = gameData->running = TRUE;
    gameData->keyboardFocus = gameData->mouseFocus = gameData->checkRenderable = FALSE;
    gameData->realFps = 0.0f;
    return Qnil;
}

static VALUE game_pause(VALUE self) {
    GameData *game; Data_Get_Struct(self, GameData, game);
    game->suspend = TRUE;
    return self;
}

static VALUE game_resume(VALUE self) {
    GameData *game; Data_Get_Struct(self, GameData, game);
    game->suspend = FALSE;
    return self;
}


static MultiGameApplication *multiGameApplication = NULL;

static void game_event_handling(VALUE self, SDL_Event *event) {
    GameData *gameData; Data_Get_Struct(self, GameData, gameData);
    if (event->window.windowID == gameData->windowID) {
        switch (event->type) {
                case SDL_WINDOWEVENT:
                    switch (event->window.event) {
                        case SDL_WINDOWEVENT_SHOWN: 
                            gameData->visible = TRUE;
                            break;
                        case SDL_WINDOWEVENT_HIDDEN: break;
                            gameData->visible = FALSE;
                            break;                       
                        case SDL_WINDOWEVENT_EXPOSED:
                            SDL_RenderPresent( gameData->renderer );
                            break;
                        case SDL_WINDOWEVENT_ENTER:
                            gameData->mouseFocus = TRUE;
                            break;
                        case SDL_WINDOWEVENT_LEAVE:
                            gameData->mouseFocus = FALSE;
                            break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            gameData->keyboardFocus = TRUE;
                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            gameData->keyboardFocus = FALSE;
                            break;
                        case SDL_WINDOWEVENT_CLOSE:
                            SDL_HideWindow(gameData->window);
                            gameData->visible = FALSE;
                            break;
                    }
                 break;   
            }
    }
}



static VALUE game_run_state_add_proc(VALUE dummy, VALUE self_and_block, int argc, VALUE argv[]) {
    rb_funcall_with_block(rb_ary_entry(self_and_block, 0), rb_intern("instance_eval"), 
        0, NULL, rb_ary_entry(self_and_block, 1));
    return Qnil;
}

static VALUE game_run(int argc, VALUE argv[], VALUE self) {
    if (rb_block_given_p()) {
        VALUE state_machine = rb_ivar_get(self, id_iv_state_machine);
        if (rb_funcall(state_machine, rb_intern("member?"), 1, ID2SYM(rb_intern("__init__"))) == Qfalse) {
            volatile VALUE proc = rb_proc_new(game_run_state_add_proc, rb_assoc_new(self, rb_block_proc()));
            rb_funcall(proc, rb_intern("call"), 0, NULL);
            rb_funcall_with_block(state_machine, rb_intern("add"), 1, (VALUE[]) { ID2SYM(rb_intern("__init__")) }, proc);
        }
    }
    rb_funcall(self, rb_intern("update"), 0);
    rb_funcall(self, rb_intern("draw"), 0);
    return Qnil;
}


static VALUE game_update_child( VALUE child, VALUE nil, int argc, VALUE argv[]) {
    rb_funcall(child, rb_intern("update"), 0);
    return rb_funcall(child, rb_intern("active?"), 0) == Qtrue ? Qfalse : Qtrue;
}

static VALUE game_update(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    if (gameData->suspend) return Qnil;
    if (gameData->collidable) { rb_funcall(self, rb_intern("each_hit"), 0); }
    rb_funcall(self, rb_intern("update_event"), 0);
    rb_funcall(rb_ivar_get(self, id_iv_state_machine), id_run, 0);
    volatile VALUE ary = rb_ivar_get(self, id_iv_pool);
    rb_block_call(ary, id_delete_if, 0, NULL, game_update_child, Qnil);
    return Qnil;
}

static VALUE game_deactive(VALUE self) {
  GameData* gameData; Data_Get_Struct(self, GameData, gameData);
  gameData->active = FALSE;
  return Qnil;
}

static VALUE game_get_event_manager(VALUE self) {
    return rb_ivar_get(self, rb_intern("@event_manager"));
} 

SDL_Renderer* GetWindowRenderer(VALUE *obj, Uint32 window_id) {
    window_id++;
    if (multiGameApplication == NULL) { return systemData->renderer; }
    MultiGameApplication* buff = multiGameApplication;
    SDL_Renderer* result = NULL;
    while (buff != NULL) {
        if (window_id == SDL_GetWindowID(buff->data->window) ) {
            result = buff->data->renderer;
            break;
        }
        buff = buff->next;
    }
    if (result == NULL) { SDL_DETAILLOG_ABORT("Window NotFound!"); }
    return result;
}

static void InitMultiWindow(int arity, int num, volatile VALUE *option, volatile VALUE *ary, VALUE game_class) {
    multiGameApplication = malloc(sizeof(MultiGameApplication));
    VALUE game_instance = rb_obj_alloc(game_class);
    rb_obj_call_init(game_instance, 0, NULL);
    GameData* gameData; Data_Get_Struct(game_instance, GameData, gameData);
    multiGameApplication->data = gameData;
    multiGameApplication->obj = game_instance;
    rb_gc_register_mark_object(multiGameApplication->obj);
    multiGameApplication->next = NULL;
    MultiGameApplication *buff = multiGameApplication;
    rb_ary_push(*ary, game_instance);
    int i;for (i = 0; i < num-1; ++i) {
        buff->next = malloc(sizeof(MultiGameApplication));
        game_instance = rb_obj_alloc(game_class);
        rb_ary_push(*ary, game_instance);
        rb_obj_call_init(game_instance, 0, NULL);
        GameData* gameData; Data_Get_Struct(game_instance, GameData, gameData);
        buff->next->data = gameData;
        multiGameApplication->obj = game_instance;
        rb_gc_register_mark_object(multiGameApplication->obj);
        buff->next->next = NULL;
        buff = buff->next;
    }

}
static void Game_Quit() {
    if (systemData->renderer != NULL) {
        SDL_DestroyRenderer(systemData->renderer);
        SDL_DestroyWindow(systemData->window);
    }
    free(systemData);
    FreeFontCache(); TTF_Quit();
    Quit_event();Quit_input();Quit_font();
    FreeSoundCache();Mix_CloseAudio();Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}

static VALUE game_get_window_id(VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    return rb_int_new(SDL_GetWindowID(gameData->window));
}

static VALUE game_screen_shot(int argc, VALUE argv[], VALUE self) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    SDL_Surface*surface = SDL_GetWindowSurface(gameData->window);
    VALUE filename, img_format = ID2SYM(rb_intern("png"));
    if (rb_scan_args(argc, argv, "11", &filename, &img_format) == 2) {
        VALUE boolean = rb_funcall(img_format, rb_intern("=="), 1, (VALUE[]) { ID2SYM(rb_intern("bmp")) });
        if (boolean == img_format) {
            SDL_SaveBMP(surface, StringValuePtr(filename));
            return Qnil;
        }
    }
    IMG_SavePNG(surface, StringValuePtr(filename));
    return Qnil;
}

static VALUE game_class_deactive(VALUE self) {
    systemData->active = FALSE;
    return Qnil;
}

static VALUE game_class_get_total_frame(VALUE klass) {
    return rb_int_new(systemData->totalFrame); 
}


static VALUE at_exit_handler(VALUE dummy, VALUE self, int argc, VALUE argv[]) {
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    if (!gameData->running) return Qnil;
    systemData->gameRunning = gameData->running = FALSE;
    ScreenData* screenData; Data_Get_Struct(rb_ivar_get(self, rb_intern("@screen")), ScreenData, screenData);
    DrawTextCache *buff = screenData->drawTextCache;
    while(buff->size != 0) { 
        free(buff->text);
        buff = buff->prev;
        free(buff->next);
    }
    free(screenData->drawTextCache);
    Game_Quit();
    return Qnil;
}

static VALUE game_loop(VALUE self) {
    SDL_Event event;
    Uint32 startTick, seekTick;
    GameData* gameData; Data_Get_Struct(self, GameData, gameData);
    ScreenData *rootScreenData; Data_Get_Struct(rb_ivar_get(self, rb_intern("@screen")), ScreenData, rootScreenData);
    SDL_Rect dst = { 0, 0,rootScreenData->rect->w,  rootScreenData->rect->h };
    SDL_ShowWindow(gameData->window);
    Uint32 fps_delay = round(1000.0f / gameData->fps), buff_real_fps = 0;
    BOOL renderable = TRUE; 

    rb_funcall_with_block(rb_mKernel, rb_intern("at_exit"), 0, NULL, rb_proc_new(at_exit_handler, self));
    
    while (gameData->active && systemData->active) {
        startTick = SDL_GetTicks();
        ++systemData->totalFrame;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: gameData->active = FALSE; break;
                case SDL_MOUSEWHEEL: GetMouseWheelY(&event.wheel.y); break;
                case SDL_JOYBUTTONUP: joyPadButtonUpManager(&event.jbutton); break;
            }
        }
        SDL_SetRenderTarget(gameData->renderer, rootScreenData->texture);
        SDL_RenderClear(gameData->renderer);
        InputManager(); 
        Update_event();
        rb_yield(self); 
        renderable = TRUE;
        if (gameData->checkRenderable) {
            seekTick = SDL_GetTicks() - startTick;
            renderable = (fps_delay >= seekTick) ? TRUE : FALSE; 
        }

        if (renderable) {
            dst.w = rootScreenData->rect->w; dst.h = rootScreenData->rect->h;
            SDL_SetRenderTarget(gameData->renderer, NULL);
            SDL_RenderCopy(gameData->renderer, rootScreenData->texture, rootScreenData->rect, &dst);
            SDL_RenderPresent(gameData->renderer);
        }
        seekTick = SDL_GetTicks() - startTick;
        if (fps_delay >= seekTick) {
            SDL_Delay(fps_delay - seekTick);
            buff_real_fps += fps_delay;
        } else {
            SDL_Delay(0);
            buff_real_fps += seekTick;
        }

        if (systemData->totalFrame % (gameData->fps >> 1) == 0) {
            int buff = ( 1000 / ((buff_real_fps*0.98f) / (gameData->fps >> 1)) ) * 10;
            gameData->realFps = buff / 10.0f;
            buff_real_fps = 0;
        }
    }
    systemData->gameRunning = gameData->running = FALSE;
    ScreenData* screenData; Data_Get_Struct(rb_ivar_get(self, rb_intern("@screen")), ScreenData, screenData);
    DrawTextCache *buff = screenData->drawTextCache;
    while(buff->size != 0) { 
        free(buff->text);
        buff = buff->prev;
        free(buff->next);
    }
    free(screenData->drawTextCache);
    Game_Quit();
    return Qnil;
}

static VALUE game_class_get_display_modes(int argc, VALUE argv[], VALUE self) {
    VALUE display_index; rb_scan_args(argc, argv, "01", &display_index);
    int displayIndex = NIL_P(display_index) ? 0 : NUM2INT(display_index);
    int num = SDL_GetNumDisplayModes( displayIndex );
    SDL_DisplayMode mode;
    volatile VALUE result_ary = rb_ary_new();
    int i; for (i = 0; i < num; ++i) {
        SDL_GetDisplayMode(displayIndex, num - i - 1, &mode);
        rb_ary_push(result_ary, rb_assoc_new(rb_int_new(mode.w), rb_int_new(mode.h)));        
    }
    return result_ary;
}


static VALUE game_class_loop(int argc, VALUE argv[], VALUE self) {
    rb_need_block();
    volatile VALUE option, block;
    if (rb_scan_args(argc, argv, "01&", &option, &block) == 0) option = rb_hash_new();
    SDL_Event event;
    Uint32 startTick, seekTick;

    volatile VALUE val = rb_hash_lookup(option, ID2SYM(rb_intern("multi")));
    int block_arity = NUM2INT( rb_proc_arity(block) );
    if ( (val != Qnil) ||  ( block_arity > 1 )) {
        volatile VALUE multi_option;
        if ( rb_type_p(val, T_HASH) ) {
            multi_option = val; val = rb_int_new(0);
        } else {
            multi_option = rb_hash_new();
        }
        volatile VALUE ary = rb_ary_new();
        InitMultiWindow(block_arity, NUM2INT(val), &multi_option, &ary, self);
        int ary_size = NUM2INT(rb_funcall(ary, rb_intern("size"), 0)) ;
        GameData * multiGameData[ary_size];
        int ary_index; for (ary_index = ary_size-1; ary_index >= 0; --ary_index) { 
            Data_Get_Struct(rb_ary_entry(ary, ary_index), GameData, multiGameData[ary_index]);
            multiGameData[ary_index]->windowID = SDL_GetWindowID(multiGameData[ary_index]->window);
            SDL_ShowWindow(multiGameData[ary_index]->window);
            char buff[64]; sprintf(buff, "no. %d ", ary_index); 
            SDL_SetWindowTitle(multiGameData[ary_index]->window, buff);
        }
        Uint32 fps_delay = round(1000.0f / systemData->fps);
        startTick = SDL_GetTicks();
        while (systemData->active) {
            systemData->totalFrame++;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) systemData->active = FALSE;
                BOOL all_deactive = TRUE;
                for (ary_index = 0; ary_index < ary_size; ++ary_index) {
                    volatile VALUE game = rb_ary_entry(ary, ary_index);
                    GameData *gameData;Data_Get_Struct(game, GameData, gameData);
                    game_event_handling(game, &event);
                    if (gameData->visible) all_deactive = FALSE;
                }
                if (all_deactive) game_class_deactive(self);
            }
            for (ary_index = 0; ary_index < ary_size; ++ary_index) {
                volatile VALUE game = rb_ary_entry(ary, ary_index);
                GameData *gameData;Data_Get_Struct(game, GameData, gameData);
                ScreenData* screenData; Data_Get_Struct(rb_ivar_get(game, rb_intern("@screen")) , ScreenData, screenData);
                SDL_SetRenderTarget(gameData->renderer, screenData->texture);
                SDL_RenderClear(gameData->renderer);
            }
            InputManager();
            rb_yield(ary);
            for (ary_index = 0; ary_index < ary_size; ++ary_index) {
                volatile VALUE game = rb_ary_entry(ary, ary_index);
                GameData *gameData;Data_Get_Struct(game, GameData, gameData);
                ScreenData* screenData; Data_Get_Struct(rb_ivar_get(game, rb_intern("@screen")) , ScreenData, screenData);
                SDL_SetRenderTarget(gameData->renderer, NULL);
                renderTexture(screenData->texture, gameData->renderer, screenData->rect);
                SDL_RenderPresent(gameData->renderer);
            }
            seekTick = SDL_GetTicks() - startTick;
            (fps_delay >= seekTick) ? SDL_Delay(fps_delay - seekTick) : SDL_Delay(0);
        }
        MultiGameApplication* buff = multiGameApplication;
        
        while (buff) {
            buff->data->running = FALSE;
            ScreenData *screenData; Data_Get_Struct(rb_ivar_get(buff->obj, rb_intern("@screen")), ScreenData, screenData);
            DrawTextCache *drawTextCacheBuff = screenData->drawTextCache->next;
            while(drawTextCacheBuff != NULL) {
                free(drawTextCacheBuff->text);
                drawTextCacheBuff = drawTextCacheBuff->next;
            }
            free(screenData->drawTextCache);
            if (buff->data->window == systemData->window) {
                systemData->window = NULL;
                systemData->renderer = NULL;
            }
            SDL_DestroyRenderer(buff->data->renderer);
            SDL_DestroyWindow(buff->data->window);
            MultiGameApplication *old_buff = buff; buff = buff->next;
            free(old_buff);
        }
        Game_Quit();
    } else {
        VALUE game_instance = rb_class_new_instance(1, (VALUE[]) { option }, self);
        rb_funcall_with_block(game_instance, rb_intern("loop"), 0, NULL, block);
    }
    return Qnil;
}

static VALUE game_class_message_box(int argc, VALUE argv[], VALUE self) {
    VALUE message;
    VALUE title = Qundef;
    VALUE flags = Qundef;
    rb_scan_args(argc, argv, "12", &message, &title, &flags);
    if (title == Qnil) title = rb_str_new2("");
    int c_flags = SDL_MESSAGEBOX_INFORMATION;
    if (flags == Qundef || SYM2ID(flags) == rb_intern("info") ) {
        c_flags = SDL_MESSAGEBOX_INFORMATION;
    } else if ( SYM2ID(flags) == rb_intern("warn") ) {
        c_flags = SDL_MESSAGEBOX_WARNING;
    } else if ( SYM2ID(flags) == rb_intern("error") ) {
        c_flags = SDL_MESSAGEBOX_ERROR;
    }
    SDL_ShowSimpleMessageBox(c_flags, StringValuePtr(title), StringValuePtr(message), NULL);
    return Qnil;
}

SystemData *systemData = NULL;

void SDLErrorAbortBody(BOOL loggable, const char *message) {
    if (loggable) {
        char full_message[512];
        if ( strlen(message) != 0) {
            sprintf(full_message, "%s - %s", message, SDL_GetError());
        } else {
            sprintf(full_message, "%s", SDL_GetError());            
        }
        P(full_message);
        LOG(full_message);
    }
    VALUE main_thread = rb_thread_main();
    VALUE current_thread = rb_thread_current();
    if(rb_equal(main_thread, current_thread) == Qfalse) {
        rb_raise(rb_eThreadError, "thread error!");
    }
    Game_Quit();
    exit(-1);
}

static void SetUpSDL() {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) == -1)
        SDL_LOG_ABORT();
    if ( !SDL_SetHint( SDL_HINT_RENDER_VSYNC, "1"))
        P("Warning: VSync not enabled!");
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
        P("Warning: Lenear texture filetering not enabled!");
    int imgFlags = IMG_INIT_PNG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) SDL_LOG_ABORT();
    if (TTF_Init() == -1) SDL_LOG_ABORT();
    if ( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 ) < 0  )
        SDL_LOG_ABORT();
    int mixFlags = MIX_INIT_OGG | MIX_INIT_MP3;
    if ((Mix_Init(mixFlags) & mixFlags) != mixFlags) SDL_LOG_ABORT();
    SetUpSound();
    SetUpSDLGameController();
    systemData = malloc(sizeof(SystemData));
    systemData->window = SDL_CreateWindow("no title.", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    BASE_WINDOW_WIDTH, BASE_WINDOW_HEIGHT, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE); 
    if (systemData->window == NULL) SDL_LOG_ABORT();
    systemData->renderer = SDL_CreateRenderer(systemData->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (systemData->renderer == NULL) {
        SDL_DestroyWindow(systemData->window);
        SDL_LOG_ABORT();
    }
    systemData->fps = BASE_FPS;
    systemData->totalFrame = 0;
    systemData->rootGameCreated = FALSE;
    systemData->active = systemData->gameRunning = TRUE;
}

static VALUE math_module_distance_sqrt(VALUE module, VALUE x, VALUE y, VALUE x2, VALUE y2) {
    int cX = NUM2INT(x); int cY = NUM2INT(y);
    int cX2 = NUM2INT(x2); int cY2 = NUM2INT(y2); 
    int a = (cX2 - cX) * (cX2 - cX); int b = (cY2 - cY) * (cY2 - cY);
    return rb_int_new(a + b);
}

static VALUE math_module_distance(VALUE module, VALUE x, VALUE y, VALUE x2, VALUE y2) {
    int cX = NUM2INT(x); int cY = NUM2INT(y);
    int cX2 = NUM2INT(x2); int cY2 = NUM2INT(y2); 
    int a = (cX2 - cX) * (cX2 - cX); int b = (cY2 - cY) * (cY2 - cY);
    return rb_int_new(sqrt(a + b));
}

static VALUE math_module_clamp(VALUE module, VALUE num, VALUE min, VALUE max) {
    double cNum = NUM2DBL(num);
    double cMin = NUM2DBL(min);
    double cMax = NUM2DBL(max);
    return cNum > cMax ? max : (cNum < cMin ? min : max);
}

static VALUE numeric_singleton_degree(VALUE radian) { return rb_int_new(NUM2DBL(radian) * 180 / MATH_PI); } 
static VALUE numeric_singleton_radian(VALUE degree) { return rb_float_new(NUM2INT(degree) * MATH_PI / 180); }

static VALUE sdludon_module_f_loop(VALUE self) {
    if (rb_block_given_p()) {
        while (TRUE) { rb_fiber_yield(0, NULL); rb_yield(Qnil);  }
    } else {
        while (TRUE) { rb_fiber_yield(0, NULL); }
    }
    return Qnil;
}

static VALUE sdludon_module_f_yield(int argc, VALUE argv[], VALUE self) {
    return rb_fiber_yield(0, argv);
}

void Init_sdl_udon(void) {
    SetUpSDL();
    volatile VALUE math_module = rb_path2class("Math");
    rb_define_module_function(math_module, "distance", math_module_distance, 4);
    rb_define_module_function(math_module, "distance_sqrt", math_module_distance_sqrt, 4);
    rb_define_module_function(math_module, "clamp", math_module_clamp, 3);
    rb_const_set(math_module, rb_intern("DOUBLE_PI"), rb_float_new(MATH_DOUBLE_PI));
    volatile VALUE numeric_class = rb_path2class("Numeric");
    rb_define_method(numeric_class, "degree", numeric_singleton_degree, 0);
    rb_define_method(numeric_class, "radian", numeric_singleton_radian, 0);
    
    volatile VALUE module_sdludon = rb_define_module("SDLUdon");
    rb_define_module_function(module_sdludon, "f_loop", sdludon_module_f_loop, 0);
    rb_define_module_function(module_sdludon, "f_yield", sdludon_module_f_yield, -1);

    volatile VALUE system_info = rb_hash_new();
    rb_hash_aset(system_info, ID2SYM(rb_intern("OS")), rb_str_freeze(rb_str_new2(SDL_GetPlatform())));
    rb_hash_aset(system_info, ID2SYM(rb_intern("MEM")), rb_str_freeze(rb_int_new(SDL_GetSystemRAM())));
    volatile VALUE cpu_info = rb_hash_new();
    rb_hash_aset(cpu_info, ID2SYM(rb_intern("CACHE")),rb_int_new(SDL_GetCPUCacheLineSize()));
    rb_hash_aset(cpu_info, ID2SYM(rb_intern("CORE")),rb_int_new(SDL_GetCPUCount()));
    rb_hash_aset(system_info, ID2SYM(rb_intern("CPU")), rb_hash_freeze(cpu_info));
    rb_const_set(module_sdludon, rb_intern("SYSTEM_INFO"), rb_hash_freeze(system_info) ) ;
    rb_const_set(module_sdludon, rb_intern("VERSION"), rb_str_freeze(rb_str_new2("3.1.0")));

    Init_screen(module_sdludon);
    Init_statemachine(module_sdludon);
    Init_manager(module_sdludon);
    Init_counter(module_sdludon);
    Init_image(module_sdludon);
    Init_font(module_sdludon);
    Init_collision(module_sdludon);
    Init_actor(module_sdludon);
    Init_scene(module_sdludon);

    VALUE class_scene = rb_path2class("SDLUdon::Scene");
    volatile VALUE class_game= rb_define_class_under(module_sdludon, "Game", class_scene);
    rb_define_singleton_method(class_game, "display_modes", game_class_get_display_modes, -1);
    rb_define_singleton_method(class_game, "loop", game_class_loop, -1);
    rb_define_singleton_method(class_game, "message_box", game_class_message_box, -1);
    rb_define_singleton_method(class_game, "deactive", game_class_deactive, 0);
    rb_define_singleton_method(class_game, "total_frame", game_class_get_total_frame, 0);
    rb_define_alloc_func(class_game, game_alloc);
    rb_define_private_method(class_game, "initialize", game_initialize, -1);
    rb_define_method(class_game, "run", game_run, -1);
    rb_define_method(class_game, "event", game_get_event_manager, 0);
    rb_define_method(class_game, "window_id", game_get_window_id, 0);
    rb_define_method(class_game, "fullscreen?", game_is_fullscreen, 0);
    rb_define_method(class_game, "fullscreen=", game_set_fullscreen, 1);
    rb_define_method(class_game, "title=", game_set_caption, 1);
    rb_define_method(class_game, "title", game_get_caption, 0);
    rb_define_method(class_game, "fps=", game_set_fps, 1);
    rb_define_method(class_game, "fps", game_get_fps, 0);
    rb_define_method(class_game, "x", game_get_x, 0);
    rb_define_method(class_game, "x2", game_get_x2, 0);
    rb_define_method(class_game, "x=", game_set_x, 1);
    rb_define_method(class_game, "y", game_get_y, 0);
    rb_define_method(class_game, "y2", game_get_y2, 0);
    rb_define_method(class_game, "y=", game_set_y, 1);
    rb_define_method(class_game, "width", game_get_width, 0);
    rb_define_method(class_game, "width=", game_set_width, 1);
    rb_define_method(class_game, "height", game_get_height, 0);
    rb_define_method(class_game, "height=", game_set_height, 1);
    rb_define_method(class_game, "screen_shot", game_screen_shot, -1);
    rb_define_method(class_game, "attach", game_attach, 1);
    rb_define_method(class_game, "attached?", game_is_attached, 1);
    rb_define_method(class_game, "loop", game_loop, 0);
    rb_define_method(class_game, "focus", game_focus, 0);
    rb_define_method(class_game, "focus?", game_is_focus, 0);
    rb_define_method(class_game, "mouse_hover?", game_is_mouse_hover, 0);
    rb_define_method(class_game, "visible?", game_is_visible, 0);
    rb_define_method(class_game, "show", game_show, 0);
    rb_define_method(class_game, "hide", game_hide, 0);
    rb_define_method(class_game, "pause", game_pause, 0);
    rb_define_method(class_game, "resume", game_resume, 0);
    rb_define_method(class_game, "active?", game_is_active, 0);
    rb_define_method(class_game, "real_fps", game_get_real_fps, 0);
    rb_define_method(class_game, "update", game_update, 0);
    rb_define_method(class_game, "deactive", game_deactive, 0);

    Init_input(module_sdludon);
    Init_sound(module_sdludon);
    Init_timeline(module_sdludon);
    Init_tween(module_sdludon);
    Init_event(module_sdludon);
    
    rb_extend_object(class_game, rb_path2class("SDLUdon::Manager"));
    rb_funcall(class_game, rb_intern("init_manage"), 0);
    rb_funcall(rb_cObject, rb_intern("include"), 1, module_sdludon); 

    id_iv_scene = rb_intern("@scene");
    id_iv_screen = rb_intern("@screen");
    id_iv_image = rb_intern("@image");
    id_iv_state_machine = rb_intern("@state_machine");
    id_iv_event_manager = rb_intern("@event_manager");
    id_iv_timeline = rb_intern("@timeline");
    id_iv_pool = rb_intern("@pool");
    id_update = rb_intern("update");
    id_each = rb_intern("each");
    id_trigger = rb_intern("trigger");
    id_collied = rb_intern("collied");
    id_draw = rb_intern("draw"); id_draw_ex = rb_intern("draw_ex");
    id_draw_actor = rb_intern("draw_actor");
    id_draw_opts = rb_intern("draw_opts");
    id_is_visible = rb_intern("visible?");
    id_run = rb_intern("run");
    id_hit = rb_intern("hit");
    id_combination = rb_intern("combination");
    id_is_active = rb_intern("active?");
    id_is_halt = rb_intern("halt?");
    id_x = rb_intern("x"); id_y = rb_intern("y");
    id_delete_if = rb_intern("delete_if");
}



void p(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    vfprintf(stderr, format, arg);
    va_end(arg);
}


FILE* safeFOPEN(const char *filename, const char *mode) {
    FILE *fp;
    if ((fp = fopen(filename, mode) ) == NULL) {
        fprintf(stderr, "file open error!\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    return fp;
}

void logger(const char* filename,const char* message, const int line, const char* srcfile) {
    time_t timer;struct tm *date;
    char time_str[256];
    timer = time(NULL);date = localtime(&timer);
    strftime(time_str, 255, " * %d/%m/%Y %H:%M", date);
    FILE *fp = safeFOPEN(filename, "a");
    fprintf(fp, "%s:%d - %s %s\n", srcfile, line, message, time_str);
    fclose(fp);
}

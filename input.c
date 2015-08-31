#include "sdl_udon_private.h"

static st_table* keyConfig = NULL;
static st_table** padConfig = NULL;
static st_table* mouseConfig = NULL;
static st_table* keyCodeMap = NULL;
static char* keyCodeTableTargetKey = NULL; // 探索用。

InputData *inputData = NULL;

typedef union _joyPad {
    SDL_Joystick  *joystick;
    SDL_GameController *gamepad;
} JoyPad;

static const char*AXIS_STRS[] = {"up", "down", "left", "right"};
static const char*BUTTON_STRS[] = {"lbtn", "mbtn", "rbtn"};

typedef struct {
    JoyPad *joyPad;
    Sint16 x,y,z;
    Uint8 axis[4]; // up, down, left, right
    RepeatData axisRepeatDatas[4];
    int buttonNum;
    Uint8 *buttons;
    char **buttonsAssign;
    RepeatData** buttonRepeatDatas;
    BOOL buttonUped;
    Uint32 upButtonsBit;
} JoyPadData;

static JoyPadData **mJoyPadData;

static inline int16_t keyNameToNum(const char*name) {
    st_data_t data;
    return (st_lookup(keyCodeMap, (st_data_t)name, &data) == 0) ? -1 : (int16_t)data;
}

static inline BOOL isKeyStateFromNum(int num, Uint8 state) {
    if (num < 0) return FALSE;
    return ((inputData->key->keyData[num-4].state & state) != 0) ? TRUE : FALSE;
}

static inline int16_t padNameToNum(const char*name) {
    int16_t result = -1;
    size_t nameLen = strlen(name);
    const char *axisStrs[] = {"up", "down", "left", "right"};
    for (int i = 0; i  < 4; ++i) { 
        if (strncmp(name, axisStrs[i], nameLen) == 0) {
            result = i;
            break;
        }
    }
    if (result == -1) {
        int16_t temp = strtol(name, NULL, 10);
        if (temp != 0) { result = temp + 4; }
    }
    return result;
}

static inline BOOL isPadStateFromNum(int num, int index, Uint8 state) {    
    if (SDL_NumJoysticks() == 0 || num < 0) return FALSE;
    JoyPadData *joy = mJoyPadData[index];
    if (num < 4) {
        return ((joy->axis[num] & state) != 0) ? TRUE : FALSE;
    } else {
        num -= 4;
        return ((joy->buttons[num] & state) != 0) ? TRUE : FALSE;
    }
}

static inline BOOL isMouseStateFromNum(int num, Uint8 state) {
    if (num < 0) return FALSE;
    return (inputData->mouse->btn_states[num] & state) != 0 ? TRUE : FALSE;
}

static inline int16_t mouseNameToNum(const char*name) {
    size_t nameLen = strlen(name);
    if (strncmp(name, "lbtn", nameLen) == 0) {
        return 0;
    } else if (strncmp(name, "mbtn", nameLen) == 0) {
        return 1;
    } else if (strncmp(name, "rbtn", nameLen) == 0) {
        return 2;
    }
    return -1;
}

inline void mouseEventManager() { 
    int x, y;
    Uint32 result = SDL_GetMouseState(&x, &y);
    MouseData *mouse = inputData->mouse;
    mouse->btn_states[0] = mouse->btn_states[0] & ~1;
    mouse->btn_states[1] = mouse->btn_states[1] & ~1;
    mouse->btn_states[2] = mouse->btn_states[2] & ~1;

    if (result) {
        for (int i = 0; i < 3; ++i) {
            if (result & SDL_BUTTON(i+1)) {
                mouse->btn_states[i] = 2 | 4;
                if (mouse->btnRepeatDatas[i]->duration == -1) { continue; }
                if (mouse->btnRepeatDatas[i]->durationCount < mouse->btnRepeatDatas[i]->duration) {
                    mouse->btnRepeatDatas[i]->durationCount++; continue;
                }
                // wait-start
                if (mouse->btnRepeatDatas[i]->wait == -1) { mouse->btn_states[i] = 2;  continue; }
                if (mouse->btnRepeatDatas[i]->waitCount <= mouse->btnRepeatDatas[i]->wait) {
                    if (mouse->btnRepeatDatas[i]->waitCount != mouse->btnRepeatDatas[i]->wait) { mouse->btn_states[i] = 2; }
                    mouse->btnRepeatDatas[i]->waitCount++;
                    continue;
                }
                // interval-start
                if (mouse->btnRepeatDatas[i]->interval == -1) { mouse->btn_states[i] = 2;  continue; }
                if (mouse->btnRepeatDatas[i]->intervalCount >= mouse->btnRepeatDatas[i]->interval) {
                    mouse->btnRepeatDatas[i]->intervalCount = 0;
                } else {
                    mouse->btn_states[i] = 2;
                    mouse->btnRepeatDatas[i]->intervalCount++;
                }
            }
        }
    } else { // up時
        for (int i = 0; i < 3; ++i) {
            if ((mouse->btn_states[i] & 2) != 0) {
                mouse->btn_states[i] = (mouse->btn_states[i] | 1) & 1;
                mouse->btnRepeatDatas[i]->durationCount = mouse->btnRepeatDatas[i]->waitCount = mouse->btnRepeatDatas[i]->intervalCount = 0;
            }
        }
    }
    mouse->moving = (x != mouse->x || y != mouse->y) ? TRUE : FALSE; 
    mouse->dx = x - mouse->x; mouse->dy = y - mouse->y;
    mouse->x = x; mouse->y = y;
}

static VALUE input_mouse_x(VALUE self) { return rb_int_new(inputData->mouse->x); }
static VALUE input_mouse_y(VALUE self) { return rb_int_new(inputData->mouse->y); }
static VALUE input_mouse_delta_x(VALUE self) { return rb_int_new(inputData->mouse->dx); }
static VALUE input_mouse_delta_y(VALUE self) { return rb_int_new(inputData->mouse->dy); }

static VALUE input_mouse_state(VALUE btn_sym, Uint8 state) {
    VALUE btn_sym_to_s = rb_funcall(btn_sym, rb_intern("to_s"), 0);
    const char* btn_str = StringValuePtr( btn_sym_to_s  );
    if (isMouseStateFromNum(mouseNameToNum(btn_str), state) ) {
        return Qtrue;
    } else {
        st_data_t data = Qnil;
        if (st_lookup(mouseConfig, (st_data_t)btn_str, &data)) {
            uint8_t keyNum = data & 0xff;
            uint8_t padNum = (data & 0xff00) >> 8;
            uint8_t padIndex = (data & 0xff0000) >> 16;
            if ((padNum != 0xff) && isPadStateFromNum(padNum, padIndex, state)) {
                return Qtrue;
            } else if ((keyNum != 0xff) && isKeyStateFromNum(keyNum, state)) {
                return Qtrue;
            }
        }
        return Qfalse;
    }
    return Qnil;
}

static VALUE input_mouse_up(VALUE self, VALUE btn_sym) { return input_mouse_state(btn_sym, 1); }
static VALUE input_mouse_down(VALUE self, VALUE btn_sym) { return input_mouse_state(btn_sym, 2); }
static VALUE input_mouse_push(VALUE self, VALUE btn_sym) { return input_mouse_state(btn_sym, 4); }

static VALUE input_mouse_moving(VALUE self) {
    return inputData->mouse->moving ? Qtrue : Qfalse;
}

inline void GetMouseWheelY(Sint32 *y) { inputData->mouse->wheel = -(*y); }

static VALUE input_mouse_wheel(VALUE self) {
    Sint32 nowWheelY = inputData->mouse->wheel;
    inputData->mouse->wheel = 0;
    return rb_int_new(nowWheelY);
}

static VALUE input_mouse_move(VALUE self, VALUE x, VALUE y) {
    SDL_WarpMouseInWindow(systemData->window, NUM2INT(x), NUM2INT(y));
    return self;
}


static inline BOOL ParseKeyBoardState(Uint8 *state, KeyData *keyData) {
    keyData->state = keyData->state & ~1; 
    if (*state == 1) { // down or push
        keyData->state = 2 | 4;
        if (keyData->repeatData->duration == -1) { return FALSE; }
        if (keyData->repeatData->durationCount < keyData->repeatData->duration) {
            keyData->repeatData->durationCount++; return FALSE;
        }
        if (keyData->repeatData->wait == -1) { keyData->state = 2;  return FALSE; } // ここからwait開始
        if (keyData->repeatData->waitCount <= keyData->repeatData->wait) {
            if (keyData->repeatData->waitCount != keyData->repeatData->wait) { keyData->state = 2; }
            keyData->repeatData->waitCount++;
            return FALSE;
        }
        if (keyData->repeatData->interval == -1) { keyData->state = 2;  return FALSE; } // interval開始
        if (keyData->repeatData->intervalCount >= keyData->repeatData->interval) {
            keyData->repeatData->intervalCount = 0;
        } else {
            keyData->state = 2;
            keyData->repeatData->intervalCount++;
        }
    } else if ((keyData->state & 2) != 0 && *state == 0) { // up
         keyData->repeatData->durationCount = keyData->repeatData->waitCount = keyData->repeatData->intervalCount = 0;
         keyData->state = keyData->state & ~(2 | 4) | 1;
    }
    return TRUE;
}

void keyEventManager() {
    const Uint8* state = SDL_GetKeyboardState(NULL);
    KeyData *keyData = inputData->key->keyData;
    int i, size = SDL_SCANCODE_KP_PERIOD - SDL_SCANCODE_A + 1;
    for (i = 0; i < size; ++i) { if (!ParseKeyBoardState((Uint8*)&state[i+4], &keyData[i])) continue; }
    for (i = size; i < size+8; ++i) { if (!ParseKeyBoardState((Uint8*)&state[i + 4 + 124], &keyData[i])) continue; }
}   

inline void joyPadButtonUpManager(SDL_JoyButtonEvent *event) {
    JoyPadData *data = mJoyPadData[event->which];
    data->buttonUped = TRUE;
    data->upButtonsBit = data->upButtonsBit | (1 << event->button);
}

void joyPadEventManager() {
    Sint16 x_move, y_move;
    int i; for (i = 0; i < SDL_NumJoysticks(); ++i) {
        JoyPadData *data = mJoyPadData[i];       
        if (SDL_IsGameController(i)) {
            /* 後で書く */
        } else {
            data->axis[0] = data->axis[0] & ~1; data->axis[1] = data->axis[1] & ~1;
            data->axis[2] = data->axis[2] & ~1; data->axis[3] = data->axis[3] & ~1;

            x_move = SDL_JoystickGetAxis(data->joyPad->joystick, 0);
            y_move = SDL_JoystickGetAxis(data->joyPad->joystick, 1);
            
            if (x_move == 0) {
                if (data->x == -1) {
                    data->axis[2] = (data->axis[2] | 1) & ~(2|4);
                    data->axisRepeatDatas[2].durationCount = data->axisRepeatDatas[2].waitCount = data->axisRepeatDatas[2].intervalCount = 0;
                }
                if (data->x == 1) {
                    data->axis[3] = (data->axis[3] | 1) & ~(2|4);
                    data->axisRepeatDatas[1].durationCount = data->axisRepeatDatas[1].waitCount = data->axisRepeatDatas[1].intervalCount = 0;
                }
                data->x = 0;
            } else if (x_move != 0) { 
                // down or push
                x_move = x_move / abs(x_move);
                if (x_move == -1) {
                    data->axis[2] =  2 | 4;
                    data->x = x_move; // old
                    if (data->axisRepeatDatas[2].duration == -1) { continue; }
                    if (data->axisRepeatDatas[2].durationCount < data->axisRepeatDatas[2].duration) {
                        data->axisRepeatDatas[2].durationCount++; continue;  
                    }
                    // wait-start
                    if (data->axisRepeatDatas[2].wait == -1) { data->axis[2] = 2;  continue; }
                    if (data->axisRepeatDatas[2].waitCount <= data->axisRepeatDatas[2].wait) {
                        if (data->axisRepeatDatas[2].waitCount != data->axisRepeatDatas[2].wait) { data->axis[2] = 2; }
                        data->axisRepeatDatas[2].waitCount++; continue;
                    }
                    // interval-start
                    if (data->axisRepeatDatas[2].interval == -1) { data->axis[2] = 2;  continue; }
                    if (data->axisRepeatDatas[2].intervalCount >= data->axisRepeatDatas[2].interval) {
                        data->axisRepeatDatas[2].intervalCount = 0;
                    } else {
                        data->axis[2] = 2;
                        data->axisRepeatDatas[2].intervalCount++;
                    }
                }

                if (x_move == 1) {
                    data->axis[3] =  2 | 4;
                    if (data->axisRepeatDatas[3].duration == -1) { continue; }
                    if (data->axisRepeatDatas[3].durationCount < data->axisRepeatDatas[3].duration) {
                        data->axisRepeatDatas[3].durationCount++; continue;  
                    }
                    // wait-start
                    if (data->axisRepeatDatas[3].wait == -1) { data->axis[3] = 2;  continue; }
                    if (data->axisRepeatDatas[3].waitCount <= data->axisRepeatDatas[3].wait) {
                        if (data->axisRepeatDatas[3].waitCount != data->axisRepeatDatas[3].wait) { data->axis[3] = 2; }
                        data->axisRepeatDatas[3].waitCount++; continue;
                    }
                    // start-interval
                    if (data->axisRepeatDatas[3].interval == -1) { data->axis[3] = 2;  continue; }
                    if (data->axisRepeatDatas[3].intervalCount >= data->axisRepeatDatas[3].interval) {
                        data->axisRepeatDatas[3].intervalCount = 0;
                    } else {
                        data->axis[3] = 2;
                        data->axisRepeatDatas[3].intervalCount++;
                    }
                }
            }

            if (y_move == 0) {
                if (data->y == -1) {
                    data->axis[0] = (data->axis[0] | 1) & ~(2|4);
                    data->axisRepeatDatas[0].durationCount = data->axisRepeatDatas[0].waitCount = data->axisRepeatDatas[0].intervalCount = 0;
                }
                if (data->y == 1) {
                    data->axis[1] = (data->axis[1] | 1) & ~(2|4);
                    data->axisRepeatDatas[1].durationCount = data->axisRepeatDatas[1].waitCount = data->axisRepeatDatas[1].intervalCount = 0;
                }
                data->y = 0;
            } else if (y_move != 0) { 
                y_move = y_move / abs(y_move);
                data->y = y_move;
                if (y_move == -1) {
                    data->axis[0] =  2 | 4;
                    if (data->axisRepeatDatas[0].duration == -1) { continue; }
                    if (data->axisRepeatDatas[0].durationCount < data->axisRepeatDatas[0].duration) {
                        data->axisRepeatDatas[0].durationCount++; continue;  
                    }
                    if (data->axisRepeatDatas[0].wait == -1) { data->axis[0] = 2;  continue; }
                    if (data->axisRepeatDatas[0].waitCount <= data->axisRepeatDatas[0].wait) {
                        if (data->axisRepeatDatas[0].waitCount != data->axisRepeatDatas[0].wait) { data->axis[0] = 2; }
                        data->axisRepeatDatas[0].waitCount++; continue;
                    }
                    if (data->axisRepeatDatas[0].interval == -1) { data->axis[0] = 2;  continue; }
                    if (data->axisRepeatDatas[0].intervalCount >= data->axisRepeatDatas[0].interval) {
                        data->axisRepeatDatas[0].intervalCount = 0;
                    } else {
                        data->axis[0] = 2;
                        data->axisRepeatDatas[0].intervalCount++;
                    }
                }

                if (y_move == 1) {
                    data->axis[1] =  2 | 4;
                    if (data->axisRepeatDatas[1].duration == -1) { continue; }
                    if (data->axisRepeatDatas[1].durationCount < data->axisRepeatDatas[1].duration) {
                        data->axisRepeatDatas[1].durationCount++; continue;  
                    }
                    if (data->axisRepeatDatas[1].wait == -1) { data->axis[1] = 2;  continue; }
                    if (data->axisRepeatDatas[1].waitCount <= data->axisRepeatDatas[1].wait) {
                        if (data->axisRepeatDatas[1].waitCount != data->axisRepeatDatas[1].wait) { data->axis[1] = 2; }
                        data->axisRepeatDatas[1].waitCount++; continue;
                    }
                    if (data->axisRepeatDatas[1].interval == -1) { data->axis[1] = 2;  continue; }
                    if (data->axisRepeatDatas[1].intervalCount >= data->axisRepeatDatas[1].interval) {
                        data->axisRepeatDatas[1].intervalCount = 0;
                    } else {
                        data->axis[1] = 2;
                        data->axisRepeatDatas[1].intervalCount++;
                    }
                }
            }
            int j;for (j = 0; j < data->buttonNum; ++j)
                data->buttons[j] = data->buttons[j] & ~1;
                 if (SDL_JoystickGetButton(data->joyPad->joystick, j)) {
                    if ( data->buttons[j] & 2 ) {
                        data->buttons[j] = 2 | 4;
                        if (data->buttonRepeatDatas[j]->duration == -1) { continue; }
                        if (data->buttonRepeatDatas[j]->durationCount < data->buttonRepeatDatas[j]->duration) {
                            data->buttonRepeatDatas[j]->durationCount++; continue;  
                        }
                        if (data->buttonRepeatDatas[j]->wait == -1) { data->buttons[j] = 2;  continue; }
                        if (data->buttonRepeatDatas[j]->waitCount <= data->buttonRepeatDatas[j]->wait) {
                            if (data->buttonRepeatDatas[j]->waitCount != data->buttonRepeatDatas[j]->wait) { data->buttons[j] = 2; }
                            data->buttonRepeatDatas[j]->waitCount++;
                            continue;
                        }
                        if (data->buttonRepeatDatas[j]->interval == -1) { data->buttons[j] = 2;  continue; }
                        if (data->buttonRepeatDatas[j]->intervalCount >= data->buttonRepeatDatas[j]->interval) {
                            data->buttonRepeatDatas[j]->intervalCount = 0;
                        } else {
                            data->buttons[j] = 2;
                            data->buttonRepeatDatas[j]->intervalCount++;
                        }
                    }
                 }
            }
            if (data->buttonUped) {
                int k;for (k = 0; k < data->buttonNum; ++k) {
                    if (data->upButtonsBit & (1 << k)) {
                        data->buttonRepeatDatas[k]->durationCount = data->buttonRepeatDatas[k]->waitCount = data->buttonRepeatDatas[k]->intervalCount = 0;
                        data->buttons[k] = data->buttons[k] & ~(2 | 4) | 1;
                    }                    
                }
                data->buttonUped = FALSE;
                data->upButtonsBit = 0;
            }
    }
}

static VALUE input_is_pad_state(int argc, VALUE argv[], Uint8 state) {
    VALUE btn_sym, index;
    if (rb_scan_args(argc, argv, "11", &btn_sym, &index )== 1) { index = rb_int_new(0);  }
    int cIndex = NUM2INT(index);
    VALUE btn_sym_to_s = rb_funcall(btn_sym, rb_intern("to_s"), 0);
    const char * btn_str = StringValuePtr(btn_sym_to_s);

    if (SDL_NumJoysticks() == 0 || cIndex >= SDL_NumJoysticks()) return Qnil;
    if ( isPadStateFromNum(padNameToNum(btn_str), cIndex, state) ) {
        return Qtrue;
    } else {
        st_data_t data = Qnil;
        if (st_lookup(padConfig[index], (st_data_t)btn_str, &data)) {
            uint8_t keyNum = data & 0xff;
            uint8_t mouseNum = (data & 0xff00) >> 8;
            if (keyNum != 255 && isKeyStateFromNum(keyNum, state)) {
                return Qtrue;
            } else if (mouseNum != 255 && isMouseStateFromNum(mouseNum, state)) {
                return Qtrue;
            }
        }
        return Qfalse;
    }
    return Qnil;

}

static VALUE input_is_pad_up(int argc, VALUE argv[], VALUE self) { return input_is_pad_state(argc, argv, 0x01); }
static VALUE input_is_pad_down(int argc, VALUE argv[], VALUE self) { return input_is_pad_state(argc, argv, 0x02); }
static VALUE input_is_pad_push(int argc, VALUE argv[], VALUE self) { return input_is_pad_state(argc, argv, 0x04); }

static VALUE input_is_pad_btns(int argc, VALUE argv[], VALUE self) {
    if (SDL_NumJoysticks() == 0) { return Qnil; }
    VALUE action, index_and_btns_syms;
    rb_scan_args(argc, argv, "1*", &action, &index_and_btns_syms);
    volatile VALUE index = rb_ary_entry(index_and_btns_syms, 0);
    if ( rb_funcall(index, rb_intern("class"), 0) == rb_cFixnum ) {
        rb_funcall(index_and_btns_syms, rb_intern("shift"), 0);
    } else {
        index = rb_int_new(0);
    }

    int size = NUM2INT(rb_funcall(index_and_btns_syms, rb_intern("size"), 0));
    VALUE sym_to_s = rb_funcall(action, rb_intern("to_s"), 0);
    const char* cAction = StringValuePtr(sym_to_s );
    size_t len = strlen(cAction);
    ID action_method_id;
    if (strncmp(cAction, "up", len) == 0) {
        action_method_id =rb_intern("pad_up?");
    } else if (strncmp(cAction, "down", len) == 0) {
        action_method_id =rb_intern("pad_down?");
    } else if (strncmp(cAction, "push", len) == 0) {
        action_method_id =rb_intern("pad_push?");
    } else {
        return Qnil;
    }
    // up, down, push
    VALUE boolean = Qtrue;
    int i; for (i = 0; i < size; ++i) {
        if (rb_funcall(self, action_method_id, 2, rb_ary_entry(index_and_btns_syms, i), index) == Qfalse) {
            boolean = Qfalse;
            break;
        }
    }
    return boolean;
}

static void InsertConfig(st_table* table, char* key, int value) {
    if (st_is_member(table, (st_data_t)key)) {
        st_insert(table, (st_data_t)key, value);
    } else {
        char *initKey = malloc(sizeof(char) * strlen(key));
        sprintf(initKey, "%s", key);
        st_insert(table, (st_data_t)initKey, value);
    }

}

static int FreeConfig(st_data_t k, st_data_t v, st_data_t arg) {
    char* keyStr = (char*)k;
    free(keyStr);
    return ST_CONTINUE;
}

static int FreeKeyCodeMap(st_data_t k, st_data_t v, st_data_t arg) {
    char* keyStr = (char*)k;
    free(keyStr);
    return ST_CONTINUE;
}


void Quit_input() {
    int i = 0;
    if (SDL_NumJoysticks() > 0) {
        int j = 0;
        for (i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                SDL_GameControllerClose(mJoyPadData[i]->joyPad->gamepad);
            } else {
                SDL_JoystickClose(mJoyPadData[i]->joyPad->joystick);
            }
            free(mJoyPadData[i]->joyPad);
            free(mJoyPadData[i]->buttons);
            for (j = 0; j < mJoyPadData[i]->buttonNum; ++j) {
                free(mJoyPadData[i]->buttonRepeatDatas[j]);
            }
            free(mJoyPadData[i]->buttonRepeatDatas);
            free(mJoyPadData[i]);
        }
    }
    free(mJoyPadData);
    
    for (i = 0; i < SDL_SCANCODE_KP_PERIOD - SDL_SCANCODE_A + 1 + 8; ++i) {
        free(inputData->key->keyData[i].repeatData);
    }
    free(inputData->key);
    for (i = 0; i < 3; ++i) free(inputData->mouse->btnRepeatDatas[i]);
    free(inputData->mouse);
    free(inputData);

    st_foreach(keyConfig, FreeConfig, (st_data_t)NULL);
    st_free_table(keyConfig);
    st_foreach(mouseConfig, FreeConfig, (st_data_t)NULL);
    st_free_table(mouseConfig);
    for (i = 0; i < SDL_NumJoysticks(); ++i) {
        st_foreach(padConfig[i], FreeConfig, (st_data_t)NULL);
        st_free_table(padConfig[i]);
    }
    free(padConfig);
    
    st_foreach(keyCodeMap, FreeKeyCodeMap, (st_data_t)NULL);
    st_free_table(keyCodeMap);

}

static inline VALUE input_key(const char* cKeyStr, Uint8 keyState) {
    st_data_t data = Qnil;
    st_data_t codeMapKey = (st_data_t)cKeyStr;    
    if (st_lookup(keyCodeMap, codeMapKey, &data) == 0) return Qnil;
    if (data >= 224) { data -= 124;}
    if ((inputData->key->keyData[data-4].state & keyState) != 0) {
        return Qtrue;
    } else {
        data = Qnil;
        if (st_lookup(keyConfig, codeMapKey, &data)) {
            uint8_t padNum = data & 0xff;
            uint8_t padIndex = (data & 0xff00) >> 8;
            uint8_t mouseNum = (data & 0xff0000) >> 16;
            if (padNum != 255 && isPadStateFromNum(padNum, padIndex, keyState)) {
                return Qtrue;
            } else if (mouseNum != 255 && isMouseStateFromNum(mouseNum, keyState)) {
                return Qtrue;
            }
        }
        return Qfalse;
    }
    return Qnil;
}

static VALUE input_key_up(VALUE self, VALUE key_sym) {
    VALUE str = rb_sym_to_s(key_sym);
    return input_key(StringValuePtr(str), 1); 
}

static VALUE input_key_down(VALUE self, VALUE key_sym) { 
    VALUE str = rb_sym_to_s(key_sym);
    return input_key(StringValuePtr(str), 2); 
}

static VALUE input_key_push(VALUE self, VALUE key_sym) { 
    VALUE str = rb_sym_to_s(key_sym);
    return input_key(StringValuePtr(str), 4); 
}

static int SearchNameFromKeyCodeMap(st_data_t k, st_data_t v, st_data_t target_code) {
    int targetCode = (int)target_code;
    int valueCode = (int)v;
    if (targetCode == valueCode) {
        keyCodeTableTargetKey = (char * )k;
        return ST_STOP;
    } else {
        return ST_CONTINUE;
    }
}

static inline const char* SearchCodeName(int code) { 
    st_foreach(keyCodeMap, SearchNameFromKeyCodeMap, (st_data_t)code); 
    return keyCodeTableTargetKey;
}

static VALUE input_up_keys(VALUE self) { 
    volatile VALUE result = rb_ary_new(); 
    KeyData *keyData = inputData->key->keyData;
    int i, size = SDL_SCANCODE_KP_PERIOD - SDL_SCANCODE_A + 1;
    for (i = 0; i < size; ++i) {
        if (keyData[i].state & 0x01) rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(SearchCodeName(i+4)), rb_intern("to_sym"), 0));
    }
    for (i = size; i < size+8; ++i) {
        if (keyData[i].state & 0x01) rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(SearchCodeName(i+4+124)), rb_intern("to_sym"), 0));
    }
    if (rb_block_given_p()) {
        for (i = 0; i < RARRAY_LEN(result); ++i) rb_yield(RARRAY_PTR(result)[i]);
    }
    return result;
}

static VALUE input_down_keys(VALUE self) {
    volatile VALUE result = rb_ary_new(); 
   KeyData *keyData = inputData->key->keyData;
    int i, size = SDL_SCANCODE_KP_PERIOD - SDL_SCANCODE_A + 1;
    for (i = 0; i < size; ++i) {
       if (keyData[i].state & 0x02) rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(SearchCodeName(i+4)), rb_intern("to_sym"), 0));
    }
    for (i = size; i < size+8; ++i) {
       if (keyData[i].state & 0x02) rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(SearchCodeName(i+4+124)), rb_intern("to_sym"), 0));
    }
    if (rb_block_given_p()) {
        for (i = 0; i < RARRAY_LEN(result); ++i) rb_yield(RARRAY_PTR(result)[i]);
    }
    return result;
}

static VALUE input_push_keys(VALUE self) { 
    volatile VALUE result = rb_ary_new();
    KeyData *keyData = inputData->key->keyData;
    int i, size = SDL_SCANCODE_KP_PERIOD - SDL_SCANCODE_A + 1;
    for (i = 0; i < size; ++i) {
        if (keyData[i].state & 0x04) rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(SearchCodeName(i+4)), rb_intern("to_sym"), 0));
    }
    for (i = size; i < size+8; ++i) {
        if (keyData[i].state & 0x04) rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(SearchCodeName(i+4+124)), rb_intern("to_sym"), 0));
    }

    if (rb_block_given_p()) {
        for (i = 0; i < RARRAY_LEN(result); ++i) rb_yield(RARRAY_PTR(result)[i]);
    }

    return result;
}

static VALUE input_push_pads(int argc, VALUE argv[], VALUE self) {
    if (SDL_NumJoysticks() == 0) return rb_ary_new();
    volatile VALUE result = rb_ary_new();
    VALUE index; rb_scan_args(argc, argv, "01", &index);
    JoyPadData* data = NIL_P(index) ? mJoyPadData[0] : mJoyPadData[NUM2INT(index)];
    VALUE str;
    int i; 
    for (i = 0; i < 4; ++i) {
        if (data->axis[i] & 0x04)
            rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(AXIS_STRS[i]), rb_intern("to_sym"), 0));
    }
    for (i = 0; i < data->buttonNum; ++i) {
        if (data->buttons[i] & 0x04) {
            str = rb_funcall(rb_fix2str(NUM2INT(i), 10), rb_intern("sub"), 2, rb_reg_new("^\\d", strlen("^\\d"), 0), rb_str_new2("_\\0") );
            rb_funcall(result, rb_intern("push"), 1, rb_funcall(str, rb_intern("to_sym"), 0));
        }
    }
    if (rb_block_given_p()) {
        for (i = 0; i < RARRAY_LEN(result); ++i) rb_yield(RARRAY_PTR(result)[i]);
    }
    return result; 
}

static VALUE input_down_pads(int argc, VALUE argv[], VALUE self) { 
    if (SDL_NumJoysticks() == 0) return rb_ary_new();
    volatile VALUE result = rb_ary_new();
    volatile VALUE index; rb_scan_args(argc, argv, "01", &index);
    if (NIL_P(index)) index = rb_int_new(0);
    JoyPadData* data = ( NIL_P(index) ? mJoyPadData[0] : mJoyPadData[NUM2INT(index)] );
    VALUE str;
    int i; 
    for (i = 0; i < 4; ++i) {
        if (data->axis[i] & 0x02)
            rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(AXIS_STRS[i]), rb_intern("to_sym"), 0));
    }
    for (i = 0; i < data->buttonNum; ++i) {
        if (data->buttons[i] & 0x02) {
            str = rb_funcall(rb_fix2str(NUM2INT(i), 10), rb_intern("sub"), 2, rb_reg_new("^\\d", strlen("^\\d"), 0), rb_str_new2("_\\0") );
            rb_funcall(result, rb_intern("push"), 1, rb_funcall(str, rb_intern("to_sym"), 0));
        }
    }
    if (rb_block_given_p()) {
        for (i = 0; i < RARRAY_LEN(result); ++i) rb_yield(RARRAY_PTR(result)[i]);
    }
    return result; 
}

static VALUE input_up_pads(int argc, VALUE argv[], VALUE self) { 
    if (SDL_NumJoysticks() == 0) return rb_ary_new();
    volatile VALUE result = rb_ary_new();
    VALUE index; rb_scan_args(argc, argv, "01", &index);
    JoyPadData* data = NIL_P(index) ? mJoyPadData[0] : mJoyPadData[NUM2INT(index)];
    VALUE str;
    int i; 
    for (i = 0; i < 4; ++i) {
        if (data->axis[i] & 0x01)
            rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(AXIS_STRS[i]), rb_intern("to_sym"), 0));
    }
    for (i = 0; i < data->buttonNum; ++i) {
        if (data->buttons[i] & 0x01) {
            str = rb_funcall(rb_fix2str(NUM2INT(i), 10), rb_intern("sub"), 2, rb_reg_new("^\\d", strlen("^\\d"), 0), rb_str_new2("_\\0") );
            rb_funcall(result, rb_intern("push"), 1, rb_funcall(str, rb_intern("to_sym"), 0));
        }
    }

    if (rb_block_given_p()) {
        for (i = 0; i < RARRAY_LEN(result); ++i) rb_yield(RARRAY_PTR(result)[i]);
    }
    return result;
}

static VALUE input_push_mouses(int argc, VALUE argv[], VALUE self) { 
    volatile VALUE result = rb_ary_new();
    MouseData *data = inputData->mouse;
    int i; 
    for (i = 0; i < 3; ++i) {
        if (data->btn_states[i] & 0x04)
            rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(BUTTON_STRS[i]) , rb_intern("to_sym"), 0));
    }
    if (rb_block_given_p()) {
        for (i = 0; i < RARRAY_LEN(result); ++i) rb_yield(RARRAY_PTR(result)[i]);
    }
    return result;
}

static VALUE input_down_mouses(VALUE self) { 
    volatile VALUE result = rb_ary_new();
    MouseData *data = inputData->mouse;
    int i; 
    for (i = 0; i < 3; ++i) {
        if (data->btn_states[i] & 0x02)
            rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(BUTTON_STRS[i]) , rb_intern("to_sym"), 0));
    }
    if (rb_block_given_p()) {
        for (i = 0; i < RARRAY_LEN(result); ++i) rb_yield(RARRAY_PTR(result)[i]);
    }
    return result; 
}

static VALUE input_up_mouses(VALUE self) { 
    volatile VALUE result = rb_ary_new();
    MouseData *data = inputData->mouse;
    int i; 
    for (i = 0; i < 3; ++i) {
        if (data->btn_states[i] & 0x01)
            rb_funcall(result, rb_intern("push"), 1, rb_funcall(rb_str_new2(BUTTON_STRS[i]) , rb_intern("to_sym"), 0));
    }
    if (rb_block_given_p()) {
        for (i = 0; i < RARRAY_LEN(result); ++i) rb_yield(RARRAY_PTR(result)[i]);
    }
    return result; 
}

static volatile VALUE sym_left = Qundef;

static VALUE input_is_x(int argc, VALUE argv[], VALUE self) {
    volatile VALUE index;
    if (rb_scan_args(argc, argv, "01", &index) == 0) { index = rb_int_new(0);  }
    JoyPadData *joy = ( (SDL_NumJoysticks() == 0) || (NUM2INT(index) >= SDL_NumJoysticks()) ) ? NULL : mJoyPadData[NUM2INT(index)];
    int x = 0;
    if ( (input_key_down(self, sym_left) == Qtrue) || (joy && joy->x == -1)) 
        x -= 1;
    if ( (input_key_down(self, ID2SYM(rb_intern("right"))) == Qtrue) || (joy && joy->x == 1)) 
        x += 1;
    return rb_int_new(x);
}

static VALUE input_is_y(int argc, VALUE argv[], VALUE self) {
    volatile VALUE index;
    if (rb_scan_args(argc, argv, "01", &index) == 0) { index = rb_int_new(0);  }
    JoyPadData *joy = (SDL_NumJoysticks() == 0 || (NUM2INT(index) >= SDL_NumJoysticks()) ) ? NULL : mJoyPadData[NUM2INT(index)];
    
    int result_y = 0;
    if ( (input_key_down(self, ID2SYM(rb_intern("up"))) == Qtrue) || (joy && joy->y == -1)) 
        result_y -= 1;
    if ( (input_key_down(self, ID2SYM(rb_intern("down"))) == Qtrue) || (joy && joy->y == 1)) 
        result_y += 1;
    return rb_int_new(result_y);
}

inline void InputManager() {
    joyPadEventManager();
    keyEventManager(); 
    SDL_PumpEvents();
    mouseEventManager();    
}

static void repeat_option_parse(int argc, VALUE argv[], VALUE self, char** cTarget, int *cDuration, int *cWait, int *cInterval, int *cIndex) {
    VALUE duration_or_option, wait, interval, target, index;
    int arity;
    if (cTarget == NULL) {
        arity = rb_scan_args(argc, argv, "13", &duration_or_option, &wait, &interval, &index);
    } else {
        arity = rb_scan_args(argc, argv, "23", &target, &duration_or_option, &wait, &interval, &index);
        VALUE temp = rb_funcall(target, rb_intern("to_s"), 0);
        *cTarget = StringValuePtr( temp );
    }
    if (arity <= 2 ) {
        *cDuration = NUM2INT(rb_hash_lookup(duration_or_option, ID2SYM(rb_intern("duration"))));
        *cWait = NUM2INT(rb_hash_lookup(duration_or_option, ID2SYM(rb_intern("wait"))));
        *cInterval = NUM2INT(rb_hash_lookup(duration_or_option, ID2SYM(rb_intern("interval"))));
    } else {
        *cDuration = NUM2INT(duration_or_option);
        *cWait = NUM2INT(wait);
        *cInterval = NUM2INT(interval);
    }
    if (cIndex != NULL && !NIL_P(index) ) { *cIndex = NUM2INT(index); }
}

static void input_key_repeat_all(int argc, VALUE argv[], VALUE self) {
    int duration, wait, interval;
    repeat_option_parse(argc, argv, self, NULL, &duration, &wait, &interval, NULL);
    KeyData *keyData = inputData->key->keyData;
    int i;for (i = 0; i < SDL_SCANCODE_KP_PERIOD - SDL_SCANCODE_A + 1; ++i) {
        if (keyData[i].repeatData->duration <= keyData[i].repeatData->durationCount)
            keyData[i].repeatData->durationCount = duration; 
        if (keyData[i].repeatData->wait < keyData[i].repeatData->waitCount)
            keyData[i].repeatData->waitCount = wait + 1;
        keyData[i].repeatData->duration = duration;
        keyData[i].repeatData->wait = wait;
        keyData[i].repeatData->interval = interval;
    }
}

static VALUE input_key_repeat(int argc, VALUE argv[], VALUE self) {
    VALUE type = rb_type(argv[0]);
    if (type == T_HASH || type == T_FIXNUM ) {
        input_key_repeat_all(argc, argv, self); return self;
    }
    char *target = NULL;
    int duration, wait, interval;
    repeat_option_parse(argc, argv, self, &target, &duration, &wait, &interval, NULL);
    st_data_t data = Qnil;
    if (st_lookup(keyCodeMap, (st_data_t)target, &data) == 0 ) return Qnil;
    KeyData *keyData = inputData->key->keyData;
    if (keyData[data-4].repeatData->duration <= keyData[data-4].repeatData->durationCount)
        keyData[data-4].repeatData->durationCount = duration; 
    if (keyData[data-4].repeatData->wait < keyData[data-4].repeatData->waitCount)
        keyData[data-4].repeatData->waitCount = wait + 1;
    keyData[data-4].repeatData->duration = duration;
    keyData[data-4].repeatData->wait = wait;
    keyData[data-4].repeatData->interval = interval;
    return self;
}

static void input_pad_repeat_all(int argc, VALUE argv[], VALUE self) {
    int duration, wait, interval, index = -1;
    repeat_option_parse(argc, argv, self, NULL, &duration, &wait, &interval, &index);
    int sticks = SDL_NumJoysticks();
    int i = 0;
    if (index != -1) {
        i = index;
        sticks = i + 1;
    }

    for (; i < sticks; ++i) {
        JoyPadData *data = mJoyPadData[i];       
        if (SDL_IsGameController(i)) {
        } else {
            int j; for (j = 0; j < 4; ++j) {
                if (data->axisRepeatDatas[j].duration <= data->axisRepeatDatas[j].durationCount)
                    data->axisRepeatDatas[j].durationCount = duration; 
                if (data->axisRepeatDatas[j].wait < data->axisRepeatDatas[j].waitCount)
                    data->axisRepeatDatas[j].waitCount = wait + 1;
                data->axisRepeatDatas[j].duration = duration;
                data->axisRepeatDatas[j].wait = wait;
                data->axisRepeatDatas[j].interval = interval;
            }
            int k;for (k = 0; k < data->buttonNum; ++k) {
                if (data->buttonRepeatDatas[k]->duration <= data->buttonRepeatDatas[k]->durationCount)
                    data->buttonRepeatDatas[k]->durationCount = duration; 
                if (data->buttonRepeatDatas[k]->wait < data->buttonRepeatDatas[k]->waitCount)
                    data->buttonRepeatDatas[k]->waitCount = wait + 1;
                data->buttonRepeatDatas[k]->duration = duration;
                data->buttonRepeatDatas[k]->wait = wait;
                data->buttonRepeatDatas[k]->interval = interval;
            }
        }
    }
}  

static VALUE input_pad_repeat(int argc, VALUE argv[], VALUE self) {
    VALUE type = rb_type(argv[0]);
    if (type == T_HASH || type == T_FIXNUM ) {
        input_pad_repeat_all(argc, argv, self); return self;
    }
    char *target = NULL;
    int duration, wait, interval, index = 0;
    repeat_option_parse(argc, argv, self, &target, &duration, &wait, &interval, &index);
    if (index >= SDL_NumJoysticks()) { return Qnil; }
    JoyPadData *data = mJoyPadData[index];
    if (SDL_IsGameController(index)) {
    } else {
        size_t targetLen = strlen(target);
        const char *axisStrs[] = {"up", "down", "left", "right"};
        int axisIndex = -1;
        int i; for (i = 0; i  < 4; ++i) { 
            if (strncmp(target, axisStrs[i], targetLen) == 0) {
                axisIndex = i;
                break;
            }
        }
        if (axisIndex == -1) {
            long btnIndex = strtol(target, NULL, 10);
            if (data->buttonRepeatDatas[btnIndex]->duration <= data->buttonRepeatDatas[btnIndex]->durationCount)
                data->buttonRepeatDatas[btnIndex]->durationCount = duration; 
            if (data->buttonRepeatDatas[btnIndex]->wait < data->buttonRepeatDatas[btnIndex]->waitCount)
                data->buttonRepeatDatas[btnIndex]->waitCount = wait + 1;
            data->buttonRepeatDatas[btnIndex]->duration = duration;
            data->buttonRepeatDatas[btnIndex]->wait = wait;
            data->buttonRepeatDatas[btnIndex]->interval = interval;
        } else {
            if (data->axisRepeatDatas[axisIndex].duration <= data->axisRepeatDatas[axisIndex].durationCount)
                data->axisRepeatDatas[axisIndex].durationCount = duration; 
            if (data->axisRepeatDatas[axisIndex].wait < data->axisRepeatDatas[axisIndex].waitCount)
                data->axisRepeatDatas[axisIndex].waitCount = wait + 1;
            data->axisRepeatDatas[axisIndex].duration = duration;
            data->axisRepeatDatas[axisIndex].wait = wait;
            data->axisRepeatDatas[axisIndex].interval = interval;
        }
    }
    return self;
}

static void input_mouse_repeat_all(int argc, VALUE argv[], VALUE self) {
    int duration, wait, interval;
    repeat_option_parse(argc, argv, self, NULL, &duration, &wait, &interval, NULL);
    int i;for (i = 0; i < 3; ++i) {
        if (inputData->mouse->btnRepeatDatas[i]->duration <= inputData->mouse->btnRepeatDatas[i]->durationCount)
            inputData->mouse->btnRepeatDatas[i]->durationCount = duration; 
        if (inputData->mouse->btnRepeatDatas[i]->wait < inputData->mouse->btnRepeatDatas[i]->waitCount)
            inputData->mouse->btnRepeatDatas[i]->waitCount = wait + 1;
        inputData->mouse->btnRepeatDatas[i]->duration = duration;
        inputData->mouse->btnRepeatDatas[i]->wait = wait;
        inputData->mouse->btnRepeatDatas[i]->interval = interval;
    }
}  

static VALUE input_mouse_repeat(int argc, VALUE argv[], VALUE self) {
    VALUE type = rb_type(argv[0]);
    if (type == T_HASH || type == T_FIXNUM ) {
        input_mouse_repeat_all(argc, argv, self); return self;
    }
    char *target = NULL;
    int duration, wait, interval;
    repeat_option_parse(argc, argv, self, &target, &duration, &wait, &interval, NULL);
    int16_t num = mouseNameToNum(target);
    if (num != -1) {
        if (inputData->mouse->btnRepeatDatas[num]->duration <= inputData->mouse->btnRepeatDatas[num]->durationCount)
            inputData->mouse->btnRepeatDatas[num]->durationCount = duration; 
        if (inputData->mouse->btnRepeatDatas[num]->wait < inputData->mouse->btnRepeatDatas[num]->waitCount)
            inputData->mouse->btnRepeatDatas[num]->waitCount = wait + 1;

        inputData->mouse->btnRepeatDatas[num]->duration = duration;
        inputData->mouse->btnRepeatDatas[num]->wait = wait;
        inputData->mouse->btnRepeatDatas[num]->interval = interval;
    }
    return self;
}

static VALUE input_set_repeat(int argc, VALUE argv[], VALUE self) {
    input_key_repeat_all(argc, argv, self);
    input_pad_repeat_all(argc, argv, self);
    input_mouse_repeat_all(argc, argv, self);
    return self;
}

static VALUE input_pad_num(VALUE self) { return rb_int_new(SDL_NumJoysticks()); }

static VALUE input_map(int argc, VALUE argv[], VALUE self) {
    int cIndex = 0;
    char *cKey = NULL, *cPad = NULL, *cMouse = NULL;
    VALUE key_or_hash, pad, index_or_mouse, mouse;
    if (rb_scan_args(argc, argv, "13", &key_or_hash, &pad, &index_or_mouse, &mouse) == 1) {
        VALUE key_str = rb_hash_lookup(key_or_hash, ID2SYM(rb_intern("key")));
        if (!NIL_P(key_str)) {
            key_str = rb_funcall(key_str, rb_intern("to_s"), 0);
            cKey = StringValuePtr(key_str);
        }
        VALUE pad_str = rb_hash_lookup(key_or_hash, ID2SYM(rb_intern("pad")));
        if (!NIL_P(pad_str)) {
            pad_str = rb_funcall(pad_str, rb_intern("to_s"), 0);
            cPad = StringValuePtr(pad_str);
        }
        index_or_mouse = rb_hash_lookup(key_or_hash, ID2SYM(rb_intern("pad_num")));
        if (!NIL_P(index_or_mouse)) {
            index_or_mouse = rb_funcall(index_or_mouse, rb_intern("to_i"), 0);
            cIndex = NUM2INT(index_or_mouse);
        }
        VALUE mouse_str = rb_hash_lookup(key_or_hash, ID2SYM(rb_intern("mouse")));
        if (!NIL_P(mouse_str)) {
            mouse_str = rb_funcall(mouse_str, rb_intern("to_s"), 0 );
            cMouse = StringValuePtr(mouse_str);
        }
    } else {
        volatile VALUE temp;
        if (!NIL_P(key_or_hash)) { temp = rb_sym_to_s(key_or_hash); cKey = StringValuePtr( temp ); }
        if (!NIL_P(pad))  { temp = rb_sym_to_s(pad); cPad = StringValuePtr( temp ); }
        if (!NIL_P(index_or_mouse)) {
            if (FIXNUM_P(index_or_mouse)) {
              cIndex = NUM2INT(index_or_mouse);
            } else {
              temp = rb_sym_to_s(index_or_mouse); cMouse = StringValuePtr( temp );              
            }
        }
        if (!NIL_P(mouse))  { temp = rb_sym_to_s(mouse); cMouse = StringValuePtr( temp ); }
    }
    int result = 0;
    if (cKey != NULL) {
        result += (cPad ? keyNameToNum(cPad) : 0xff);
        result += cIndex << 8;
        result += (cMouse ? (mouseNameToNum(cMouse) << 16) : (0xff << 16) ) ;
        InsertConfig(keyConfig, cKey, result);
    }
    if (cPad != NULL && SDL_NumJoysticks() > 0) {
        result = 0; result += (cKey ? keyNameToNum(cKey) : 0xff);
        result += (cMouse ? (mouseNameToNum(cMouse) << 8) : (0xff << 8) ) ;
        InsertConfig(padConfig[cIndex], cPad, result);
    }
    if (cMouse != NULL) {
        result = 0; result += (cKey ? keyNameToNum(cKey) : 0xff);
        result += (cPad ? (padNameToNum(cPad) << 8) : (0xff << 8) ) ;
        result += cIndex << 16;
        InsertConfig(mouseConfig, cMouse, result);
    }
    return self;
}

void SetUpSDLGameController() {
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    if (SDL_NumJoysticks() == 0) { return; }
    SDL_JoystickEventState(SDL_ENABLE);
    mJoyPadData = malloc(sizeof(JoyPadData*) * SDL_NumJoysticks());
    int i;for (i = 0; i < SDL_NumJoysticks(); ++i) {
        mJoyPadData[i] = malloc(sizeof(JoyPadData) );
        mJoyPadData[i]->buttonUped = FALSE;
        mJoyPadData[i]->upButtonsBit = 0;
        mJoyPadData[i]->joyPad = malloc(sizeof(JoyPad));
        if (SDL_IsGameController(i)) {
            mJoyPadData[i]->joyPad->gamepad = SDL_GameControllerOpen(i);
            if (!mJoyPadData[i]->joyPad->gamepad) SDL_LOG_ABORT();
            mJoyPadData[i]->buttonNum = SDL_JoystickNumButtons(SDL_GameControllerGetJoystick(mJoyPadData[i]->joyPad->gamepad)) ;
        } else {
            mJoyPadData[i]->joyPad->joystick = SDL_JoystickOpen(i);
            if (!mJoyPadData[i]->joyPad->joystick) SDL_LOG_ABORT();
            mJoyPadData[i]->buttonNum = SDL_JoystickNumButtons(mJoyPadData[i]->joyPad->joystick ) ;
        }
        mJoyPadData[i]->buttonRepeatDatas = malloc(sizeof(RepeatData*) * mJoyPadData[i]->buttonNum);
        mJoyPadData[i]->buttons = malloc(sizeof(Uint8) * mJoyPadData[i]->buttonNum);
        
        mJoyPadData[i]->x = mJoyPadData[i]->y = 0;

        int j; for (j = 0; j < 4; ++j) {
            mJoyPadData[i]->axis[j] = 0;
            mJoyPadData[i]->axisRepeatDatas[j].duration = 1;
            mJoyPadData[i]->axisRepeatDatas[j].wait = -1;
            mJoyPadData[i]->axisRepeatDatas[j].interval = 0;
            mJoyPadData[i]->axisRepeatDatas[j].durationCount = 0;
            mJoyPadData[i]->axisRepeatDatas[j].waitCount = 0;
            mJoyPadData[i]->axisRepeatDatas[j].intervalCount = 0;
        }
        int k; for (k = 0; k < mJoyPadData[i]->buttonNum; ++k) {
            mJoyPadData[i]->buttons[k] = 0;
            mJoyPadData[i]->buttonRepeatDatas[k] = malloc(sizeof(RepeatData));
            mJoyPadData[i]->buttonRepeatDatas[k]->duration = 1;
            mJoyPadData[i]->buttonRepeatDatas[k]->wait = -1;
            mJoyPadData[i]->buttonRepeatDatas[k]->interval = 0;
            mJoyPadData[i]->buttonRepeatDatas[k]->durationCount = 0;
            mJoyPadData[i]->buttonRepeatDatas[k]->waitCount = 0;
            mJoyPadData[i]->buttonRepeatDatas[k]->intervalCount = 0;
        }
    }
}

void Init_input(VALUE parent) {
    VALUE input_module = rb_define_module_under(parent, "Input");
    rb_define_module_function(input_module, "pad_num", input_pad_num, 0);  
    rb_define_module_function(input_module, "set_repeat", input_set_repeat, -1);  
    rb_define_module_function(input_module, "key_repeat", input_key_repeat, -1);  
    rb_define_module_function(input_module, "pad_repeat", input_pad_repeat, -1);  
    rb_define_module_function(input_module, "mouse_repeat", input_mouse_repeat, -1);  
    rb_define_module_function(input_module, "config", input_map, -1);
    rb_define_module_function(input_module, "key_up?", input_key_up, 1);
    rb_define_module_function(input_module, "key_down?", input_key_down, 1);
    rb_define_module_function(input_module, "key_push?", input_key_push, 1);
//    rb_define_module_function(input_module, "keys?", input_keys, -1);
    rb_define_module_function(input_module, "push_keys", input_push_keys, 0);
    rb_define_module_function(input_module, "down_keys", input_down_keys, 0);
    rb_define_module_function(input_module, "up_keys", input_up_keys, 0);
    rb_define_module_function(input_module, "push_pads", input_push_pads, -1);
    rb_define_module_function(input_module, "down_pads", input_down_pads, -1);
    rb_define_module_function(input_module, "up_pads", input_up_pads, -1);
    rb_define_module_function(input_module, "push_mouses", input_push_mouses, 0);
    rb_define_module_function(input_module, "down_mouses", input_down_mouses, 0);
    rb_define_module_function(input_module, "up_mouses", input_up_mouses, 0);

    rb_define_module_function(input_module, "mouse_x", input_mouse_x, 0);
    rb_define_module_function(input_module, "mouse_y", input_mouse_y, 0);
    rb_define_module_function(input_module, "mouse_delta_x", input_mouse_delta_x, 0);
    rb_define_module_function(input_module, "mouse_delta_y", input_mouse_delta_y, 0);
    rb_define_module_function(input_module, "mouse_up?", input_mouse_up, 1);
    rb_define_module_function(input_module, "mouse_down?", input_mouse_down, 1);
    rb_define_module_function(input_module, "mouse_push?", input_mouse_push, 1);
    rb_define_module_function(input_module, "mouse_moving?", input_mouse_moving, 0);
    rb_define_module_function(input_module, "mouse_wheel", input_mouse_wheel, 0);
    rb_define_module_function(input_module, "mouse_move", input_mouse_move, 2);
    rb_define_module_function(input_module, "pad_up?", input_is_pad_up, -1);    
    rb_define_module_function(input_module, "pad_down?", input_is_pad_down, -1);
    rb_define_module_function(input_module, "pad_push?", input_is_pad_push, -1);
    rb_define_module_function(input_module, "pad_btns?", input_is_pad_btns, -1);
    rb_define_module_function(input_module, "x", input_is_x, -1);
    rb_define_module_function(input_module, "y", input_is_y, -1);

    inputData = ALLOC(InputData);
    inputData->key = ALLOC(KeyBoardData);

    inputData->mouse = ALLOC(MouseData);
    inputData->mouse->x = inputData->mouse->y = 0;
    inputData->mouse->dx = inputData->mouse->dy = 0;
    inputData->mouse->wheel = 0;
    int i;
    for (i = 0; i < 3; ++i) {
        inputData->mouse->btn_states[i] = 0;
        inputData->mouse->btnRepeatDatas[i] = malloc(sizeof(RepeatData));
        inputData->mouse->btnRepeatDatas[i]->duration = 1;
        inputData->mouse->btnRepeatDatas[i]->wait = -1;
        inputData->mouse->btnRepeatDatas[i]->interval = 0;
        RepeatData *repeatData =  inputData->mouse->btnRepeatDatas[i];
        repeatData->durationCount = repeatData->waitCount = repeatData->intervalCount = 0;
    }

    sym_left = ID2SYM(rb_intern("left"));
    const char* scanCodeNames[] = {
       "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
        "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
        "_1", "_2", "_3", "_4", "_5", "_6", "_7", "_8", "_9", "_0",
        "enter", "escape", "backspace", "tab", "space", "minus",
        "equals", "leftbracket", "rightbracket", "backslash", "nonushash",
        "semicolon", "apostrophe", "grave", "comma", "period", "slash", "capslock",
        "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12",
        "printscreen", "scrolllock", "pause", "insert", "home", 
        "pageup", "delete", "end", "pagedown", "right", "left", "down", "up",
        "numlockclear", "kp_divide", "kp_multiply", "kp_minus", "kp_plus", "kp_enter",
        "kp_1", "kp_2", "kp_3", "kp_4", "kp_5", "kp_6", "kp_7", "kp_8", "kp_9", "kp_0", "kp_period",
        "lctrl", "lshift", "lalt", "lgui", "rctrl", "rshift", "ralt", "rgui"
    };

    int size = SDL_SCANCODE_KP_PERIOD - SDL_SCANCODE_A + 1;
    keyCodeMap = st_init_strtable_with_size(size + 8);
    for (i = 0; i < size + 8; ++i) {
        KeyData *keyData = &inputData->key->keyData[i];
        keyData->state = 0;
        keyData->repeatData = malloc(sizeof(RepeatData));
        RepeatData *repeatData = keyData->repeatData; 
        repeatData->durationCount = repeatData->waitCount = 0;
        repeatData->interval = repeatData->intervalCount = 0;
        repeatData->duration = 1; repeatData->wait = -1;
        char *key = malloc(sizeof(char) * strlen(scanCodeNames[i]) + 1);
        sprintf(key, "%s", scanCodeNames[i]);
        st_add_direct(keyCodeMap, (st_data_t)key, (i+4));
    }
    const char* extScanCodeNames[] = { "lctrl", "lshift", "lalt", "lgui", "rctrl",
        "rshift", "ralt", "rgui" };
    for (i = 224; i < 231+1; ++i ) {
        char *key = malloc(sizeof(char) * strlen(extScanCodeNames[i-224]) + 1 );
        sprintf(key, "%s", extScanCodeNames[i-224]);
        st_add_direct(keyCodeMap, (st_data_t)key, i); 
    }

    keyConfig = st_init_strtable();
    padConfig = malloc(sizeof(st_table) * SDL_NumJoysticks());
    int j;for (j = 0; j < SDL_NumJoysticks(); ++j) {
        padConfig[j] = st_init_strtable();
    } 
    mouseConfig = st_init_strtable();
}
#include "sdl_udon_private.h"


void screen_dfree(ScreenData *screen) {
        SDL_free(screen->rect);
        free(screen);
}

static VALUE screen_alloc(VALUE klass) {
    ScreenData *screen = ALLOC(ScreenData);
    return Data_Wrap_Struct(klass, 0, screen_dfree, screen);
}

static VALUE screen_initialize(int argc, VALUE argv[], VALUE self) {
    ScreenData *screen; Data_Get_Struct(self, ScreenData, screen);
    VALUE width, height, window_id;
    if (rb_scan_args(argc, argv, "21", &width, &height, &window_id) == 2) {
        window_id = rb_int_new(0);
        screen->renderer = systemData->renderer;
    } else {
        screen->renderer = GetWindowRenderer(&self, NUM2INT(window_id));
        if (screen->renderer == NULL) { SDL_DETAILLOG_ABORT("Window NotFound!"); }
    }
    screen->attach_id = NUM2INT(window_id) + 1;
    screen->texture = SDL_CreateTexture(screen->renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_TARGET, FIX2INT(width), FIX2INT(height));
    screen->rect = SDL_malloc(sizeof(SDL_Rect));
    screen->rect->x = 0; screen->rect->y = 0;
    SDL_QueryTexture(screen->texture, NULL, NULL, &screen->rect->w, &screen->rect->h);
    screen->drawTextCache = malloc(sizeof(DrawTextCache));
    screen->drawTextCache->size = screen->drawTextCache->textureByteSize = 0;
    screen->drawTextCache->next = NULL;
    screen->drawTextCache->prev = NULL;
    screen->drawTextCacheSize = 0;
    return Qnil;
}

static VALUE screen_get_x(VALUE self) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    return rb_int_new(screenData->rect->x);
}

static VALUE screen_set_x(VALUE self, VALUE x) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    screenData->rect->x = NUM2INT(x);
    return Qnil;
}

static VALUE screen_set_y(VALUE self, VALUE y) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    screenData->rect->y = NUM2INT(y);
    return Qnil;
}

static VALUE screen_get_y(VALUE self) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    return rb_int_new(screenData->rect->y);
}

static VALUE screen_set_blend(VALUE self, VALUE blend) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    volatile VALUE blend_str = rb_funcall(blend, rb_intern("to_s"), 0);
    const char* cBlend = StringValuePtr(blend_str);
    int len = strlen(cBlend);
    SDL_BlendMode mode;
    if (strncmp(cBlend, "blend", len) == 0) {
        mode = SDL_BLENDMODE_BLEND;
    } else if (strncmp(cBlend, "add", len) == 0) {
        mode = SDL_BLENDMODE_ADD;
    } else if (strncmp(cBlend, "mod", len) == 0) {
        mode = SDL_BLENDMODE_MOD;
    } else {
        mode = SDL_BLENDMODE_NONE;
    }
    SDL_SetRenderDrawBlendMode(screenData->renderer, mode);
    return blend;
}

static VALUE screen_get_blend(VALUE self) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    SDL_BlendMode mode;
    SDL_GetRenderDrawBlendMode(screenData->renderer, &mode);
    switch (mode) {
        case SDL_BLENDMODE_ADD:
            return ID2SYM(rb_intern("add"));
        case SDL_BLENDMODE_BLEND:
            return ID2SYM(rb_intern("blend"));
        case SDL_BLENDMODE_MOD:
            return ID2SYM(rb_intern("mod"));
        case SDL_BLENDMODE_NONE:
            return ID2SYM(rb_intern("none"));
        default:
            return ID2SYM(rb_intern("unknown"));        
    }
}

static VALUE screen_get_width(VALUE self) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    return rb_int_new(screenData->rect->w);
}

static VALUE screen_get_height(VALUE self) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    return rb_int_new(screenData->rect->h);
}

static VALUE screen_bgcolor(VALUE self, VALUE color) {
    Uint8 r = NUM2INT(RARRAY_PTR(color)[0]);
    Uint8 g = NUM2INT(RARRAY_PTR(color)[1]);
    Uint8 b = NUM2INT(RARRAY_PTR(color)[2]);
    Uint8 a = RARRAY_LENINT(color) == 4 ? NUM2INT(RARRAY_PTR(color)[3]) : 255; 
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    SDL_SetRenderDrawColor(screenData->renderer, r, g, b, a);
    return self;
}

static VALUE screen_to_image(VALUE self) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    volatile VALUE image = rb_class_new_instance(2, 
        (VALUE[]){ rb_int_new(screenData->rect->w),rb_int_new(screenData->rect->h) }, rb_path2class("SDLUdon::Image"));

    ImageData* dstData; Data_Get_Struct(image, ImageData, dstData);

    int srcWidth = screenData->rect->w;
    int srcHeight = screenData->rect->h;
    int x = 0, y = 0, srcX = 0, srcY = 0;
    void *pixels;
    int pitch, srcPitch = srcWidth * sizeof(Uint32);
    SDL_LockTexture(dstData->texture, dstData->rect, &pixels, &pitch);

    Uint32 *buff = pixels;
    Uint8 *srcBuff = malloc( srcWidth * srcHeight * sizeof(Uint32));
    SDL_RenderReadPixels(screenData->renderer, screenData->rect,
        SDL_PIXELFORMAT_ARGB8888, srcBuff, srcPitch);

    SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);

    int i, diff_w = 0, diff_h = 0; Uint32 color;
    if (srcX + srcWidth > screenData->rect->w)
        srcWidth -= ( (srcX + srcWidth) - screenData->rect->w );
    if (srcY + srcHeight > screenData->rect->h)
        srcHeight -= ( (srcY + srcHeight) - screenData->rect->h );
    if (srcX < 0) srcX = 0;
    if (srcY < 0) srcY = 0;
    
    diff_w = x + srcWidth; diff_h = y + srcHeight;
    if (diff_w > dstData->rect->w) diff_w = dstData->rect->w; 
    if (diff_h > dstData->rect->w) diff_h = dstData->rect->h;
    if (x < 0) x = 0;if (y < 0) y = 0;

    Uint32 index;
    for (i = 0; i < diff_w*diff_h; ++i) {
        index = i * 4;
        color = SDL_MapRGBA(format, srcBuff[index+2], srcBuff[index+1], srcBuff[index], srcBuff[index+3]);
        buff[i] = color;
    }
    free(format);
    free(srcBuff);
    SDL_UnlockTexture(dstData->texture);
    return image;
}

static VALUE screen_draw_line(VALUE self, VALUE x, VALUE y, VALUE x2, VALUE y2) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    SDL_RenderDrawLine(screenData->renderer, 
        NUM2INT(x), NUM2INT(y), NUM2INT(x2), NUM2INT(y2));
    return self;
}


static VALUE screen_draw_lines(int argc, VALUE argv[], VALUE self) {
    VALUE ary;
    rb_scan_args(argc, argv, "*", &ary);
    int i, count, len = RARRAY_LENINT(ary);
    SDL_Point * points;    
    if (RB_TYPE_P(RARRAY_PTR(ary)[0], T_ARRAY)) {
        count = len;
        points = malloc(sizeof(SDL_Point)*count );
        for (i = 0; i < len; ++i) {
            VALUE x = rb_ary_entry(RARRAY_PTR(ary)[i], 0);
            VALUE y = rb_ary_entry(RARRAY_PTR(ary)[i], 1);

            points[i].x = NUM2INT(x); points[i].y = NUM2INT(y); 
        }
    } else {
        count = len/2;
        points = malloc(sizeof(SDL_Point)*count);
        for (i = 0;i < len; i+=2) {
            VALUE x = RARRAY_PTR(ary)[i];
            VALUE y = RARRAY_PTR(ary)[i+1];
            points[i/2].x = NUM2INT(x); points[i/2].y = NUM2INT(y); 
        }
    }
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    SDL_RenderDrawLines(screenData->renderer, points, count);
    free(points);
    return self;
}

static VALUE screen_get_pixel(VALUE self, VALUE x, VALUE y) {
    ScreenData *screen; Data_Get_Struct(self, ScreenData, screen);
    SDL_Rect rect;
    rect.x = 0; rect.y = 0;
    rect.w = screen->rect->w; rect.h = screen->rect->w;
    uint32_t *pixels = malloc(sizeof(uint32_t) * rect.w * rect.h);
    SDL_SetRenderTarget(screen->renderer, NULL);
    SDL_RenderReadPixels(screen->renderer, NULL, 0, &pixels, rect.w * 4);
    Uint32 result = pixels[0];
    free(pixels);
    return rb_int_new(result);
}

static VALUE screen_set_pixel(int argc, VALUE argv[], VALUE self) {
    VALUE ary;
    rb_scan_args(argc, argv, "*", &ary);
    int i, count, len = RARRAY_LENINT(ary);
    SDL_Point * points;    
    if (RB_TYPE_P(RARRAY_PTR(ary)[0], T_ARRAY)) {
        count = len;
        points = malloc(sizeof(SDL_Point)*count );
        for (i = 0; i < len; ++i) {
            VALUE x = rb_ary_entry(RARRAY_PTR(ary)[i], 0);
            VALUE y = rb_ary_entry(RARRAY_PTR(ary)[i], 1);
            points[i].x = NUM2INT(x); points[i].y = NUM2INT(y); 
        }
    } else {
        count = len/2;
        points = malloc(sizeof(SDL_Point)*count);
        for (i = 0;i < len; i+=2) {
            VALUE x = RARRAY_PTR(ary)[i];
            VALUE y = RARRAY_PTR(ary)[i+1];
            points[i/2].x = NUM2INT(x); points[i/2].y = NUM2INT(y); 
        }
    }    
    return self;
}

static VALUE screen_draw_rect(VALUE self, VALUE x, VALUE y, VALUE w, VALUE h) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    SDL_Rect rect;
    rect.x = NUM2INT(x); rect.y = NUM2INT(y);
    rect.w = NUM2INT(w); rect.h = NUM2INT(h);
    SDL_RenderDrawRect(screenData->renderer, &rect);
    return self;
}

static VALUE screen_draw_rects(int argc, VALUE argv[], VALUE self) {
    VALUE ary;
    rb_scan_args(argc, argv, "*", &ary);
    int i, count, len = RARRAY_LENINT(ary);
    SDL_Rect *rects;  
    if (RB_TYPE_P(RARRAY_PTR(ary)[0], T_ARRAY)) {
        count = len;
        rects = malloc(sizeof(SDL_Rect)*count );
        for (i = 0; i < len; ++i) {
            VALUE x = rb_ary_entry(RARRAY_PTR(ary)[i], 0);
            VALUE y = rb_ary_entry(RARRAY_PTR(ary)[i], 1);
            VALUE w = rb_ary_entry(RARRAY_PTR(ary)[i], 2);
            VALUE h = rb_ary_entry(RARRAY_PTR(ary)[i], 3);

            rects[i].x = NUM2INT(x); rects[i].y = NUM2INT(y); 
            rects[i].w = NUM2INT(w); rects[i].h = NUM2INT(h); 
        }
    } else {
        count = len/4;
        rects = malloc(sizeof(SDL_Rect)*count);
        for (i = 0;i < len; i+=4) {
            VALUE x = RARRAY_PTR(ary)[i];
            VALUE y = RARRAY_PTR(ary)[i+1];
            VALUE w = RARRAY_PTR(ary)[i+2];
            VALUE h = RARRAY_PTR(ary)[i+3];
            rects[i/4].x = NUM2INT(x); rects[i/4].y = NUM2INT(y); 
            rects[i/4].w = NUM2INT(w); rects[i/4].h = NUM2INT(h); 
        }
    }
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    SDL_RenderDrawRects(screenData->renderer, rects, count);
    free(rects);

    return self;
}

static VALUE screen_draw_fill_rect(VALUE self, VALUE x, VALUE y, VALUE w, VALUE h) {
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    SDL_Rect rect;
    rect.x = NUM2INT(x); rect.y = NUM2INT(y);
    rect.w = NUM2INT(w); rect.h = NUM2INT(h);
    SDL_RenderFillRect(screenData->renderer, &rect);
    return self;
}

static VALUE screen_draw_fill_rects(int argc, VALUE argv[], VALUE self) {
    VALUE ary;
    rb_scan_args(argc, argv, "*", &ary);
    int i, count, len = RARRAY_LENINT(ary);
    SDL_Rect *rects;  
    if (RB_TYPE_P(RARRAY_PTR(ary)[0], T_ARRAY)) {
        count = len;
        rects = malloc(sizeof(SDL_Rect)*count );
        for (i = 0; i < len; ++i) {
            VALUE x = rb_ary_entry(RARRAY_PTR(ary)[i], 0);
            VALUE y = rb_ary_entry(RARRAY_PTR(ary)[i], 1);
            VALUE w = rb_ary_entry(RARRAY_PTR(ary)[i], 2);
            VALUE h = rb_ary_entry(RARRAY_PTR(ary)[i], 3);

            rects[i].x = NUM2INT(x); rects[i].y = NUM2INT(y); 
            rects[i].w = NUM2INT(w); rects[i].h = NUM2INT(h); 
        }
    } else {
        count = len/4;
        rects = malloc(sizeof(SDL_Rect)*count);
        for (i = 0;i < len; i+=4) {
            VALUE x = RARRAY_PTR(ary)[i];
            VALUE y = RARRAY_PTR(ary)[i+1];
            VALUE w = RARRAY_PTR(ary)[i+2];
            VALUE h = RARRAY_PTR(ary)[i+3];
            rects[i/4].x = NUM2INT(x); rects[i/4].y = NUM2INT(y); 
            rects[i/4].w = NUM2INT(w); rects[i/4].h = NUM2INT(h); 
        }
    }
    ScreenData* screenData; Data_Get_Struct(self, ScreenData, screenData);
    SDL_RenderFillRects(screenData->renderer, rects, count);
    free(rects);

    return self;
}

static VALUE screen_draw(VALUE self, VALUE image, VALUE x, VALUE y) {
    ImageData *imageData; Data_Get_Struct(image, ImageData, imageData);
    ScreenData *screen; Data_Get_Struct(self, ScreenData, screen);
    SDL_SetRenderTarget(screen->renderer, screen->texture);
    SDL_Rect rect;
    rect.x = NUM2INT(x); rect.y = NUM2INT(y); 
    rect.w = imageData->rect->w; rect.h = imageData->rect->h;
    SDL_RenderCopy(screen->renderer, imageData->texture, imageData->rect, &rect);
    return Qnil;
}

static VALUE screen_draw_ex(int argc, VALUE argv[],VALUE self) {
    volatile VALUE image, x, y, option;
    rb_scan_args(argc, argv, "31", &image, &x, &y, &option);
    if (NIL_P(option)) return screen_draw(self, image, x, y);
    ImageData *imageData; Data_Get_Struct(image, ImageData, imageData);
    ScreenData *screen; Data_Get_Struct(self, ScreenData, screen);
    SDL_SetRenderTarget(screen->renderer, screen->texture);
    SDL_Rect rect;
    SDL_Rect srcRect = { imageData->rect->x, imageData->rect->y, imageData->rect->w, imageData->rect->h };
    VALUE src_x = rb_hash_lookup(option, ID2SYM(rb_intern("src_x")));
    if (!NIL_P(src_x)) srcRect.x = NUM2INT(src_x);
    VALUE src_y = rb_hash_lookup(option, ID2SYM(rb_intern("src_y")));
    if (!NIL_P(src_y)) srcRect.y = NUM2INT(src_y);
    VALUE src_h = rb_hash_lookup(option, ID2SYM(rb_intern("src_h")));
    if (!NIL_P(src_h)) srcRect.h = NUM2INT(src_h);
    VALUE src_w = rb_hash_lookup(option, ID2SYM(rb_intern("src_w")));
    if (!NIL_P(src_w)) srcRect.w = NUM2INT(src_w);
    rect.x = NUM2INT(x); rect.y = NUM2INT(y);
    rect.w = imageData->rect->w; rect.h = imageData->rect->h;
    VALUE dst_w = rb_hash_lookup(option, ID2SYM(rb_intern("dst_w")));
    if (!NIL_P(dst_w)) rect.w = NUM2INT(dst_w);
    VALUE dst_h = rb_hash_lookup(option, ID2SYM(rb_intern("dst_h")));
    if (!NIL_P(dst_h)) rect.h = NUM2INT(dst_h);
    VALUE v_angle = rb_hash_lookup(option, ID2SYM(rb_intern("angle")));
    double angle = (NIL_P(v_angle) ? 0 : NUM2DBL(v_angle));
    VALUE v_scale_x = rb_hash_lookup(option, ID2SYM(rb_intern("scale_x")));
    VALUE v_scale_y = rb_hash_lookup(option, ID2SYM(rb_intern("scale_y")));
    float scale_x = (NIL_P(v_scale_x) ? 1.0f : NUM2DBL(v_scale_x));
    float scale_y = (NIL_P(v_scale_y) ? 1.0f : NUM2DBL(v_scale_y));
    VALUE v_center_x = rb_hash_lookup(option, ID2SYM(rb_intern("center_x")));
    VALUE v_center_y = rb_hash_lookup(option, ID2SYM(rb_intern("center_y")));
    SDL_Point center;
    center.x = (NIL_P(v_center_x) ?  (srcRect.w / 2) : NUM2INT(v_center_x));
    center.y = (NIL_P(v_center_y) ?  (srcRect.h / 2) : NUM2INT(v_center_y));
    
    VALUE v_flip = rb_hash_lookup(option, ID2SYM(rb_intern("flip")));
    SDL_RendererFlip flip = SDL_FLIP_NONE;

    if (!NIL_P(v_flip)) {
        const char* flipStr = rb_id2name(SYM2ID(v_flip));
        size_t str_length = strlen(flipStr);
        if (strncmp(flipStr, "horizontal", str_length) == 0) {
            flip = SDL_FLIP_HORIZONTAL; 
        } else if (strncmp(flipStr, "vertical", str_length) == 0) {
            flip = SDL_FLIP_VERTICAL; 
        }
    }
    SDL_RenderSetScale(screen->renderer, scale_x, scale_y);
    rect.x /= scale_x;  rect.y /= scale_y;
    SDL_RenderCopyEx(screen->renderer, imageData->texture, &srcRect, &rect, angle, &center, flip);
    SDL_RenderSetScale(screen->renderer, 1.0f, 1.0f);

    return Qnil;
}

static VALUE screen_draw_actor(VALUE self, VALUE actor) {
    ActorData *actorData; Data_Get_Struct(actor, ActorData, actorData);
    if (!actorData->visible) return Qnil;
    VALUE image = rb_ivar_get(actor, rb_intern("@image"));
    ImageData *imageData; Data_Get_Struct(image, ImageData, imageData);

    ScreenData *screen; Data_Get_Struct(self, ScreenData, screen);
    SDL_SetRenderTarget(screen->renderer, screen->texture);
    SDL_Rect rect;
    rect.x = NUM2INT(actorData->x); rect.y = NUM2INT(actorData->y); 
    rect.w = imageData->rect->w; rect.h = imageData->rect->h;
    SDL_RenderCopy(screen->renderer, imageData->texture, imageData->rect, &rect);
    return Qnil;
}



static VALUE screen_draw_tiles(int argc, VALUE argv[], VALUE self) {
    VALUE images, map_ary, dst_x, dst_y, option;
    
    rb_scan_args(argc, argv, "23", &images, &map_ary, &dst_x, &dst_y, &option);
    int cDstX = (NIL_P(dst_x)) ? 0 : NUM2INT(dst_x); 
    int cDstY = (NIL_P(dst_y)) ? 0 : NUM2INT(dst_y);
    int x, y,cSrcX = 0, cSrcY = 0, cWidth = 0, cHeight = 0;

    int cDiffX = 0, cDiffY = 0;
    if (!NIL_P(option)) {
        VALUE src_x = rb_hash_lookup(option, ID2SYM(rb_intern("src_x")));
        if (!NIL_P(src_x)) cSrcX = NUM2INT(src_x);
        VALUE src_y = rb_hash_lookup(option, ID2SYM(rb_intern("src_y")));
        if (!NIL_P(src_y)) cSrcY = NUM2INT(src_y);
        VALUE width = rb_hash_lookup(option, ID2SYM(rb_intern("width")));
        if (!NIL_P(width)) cWidth = NUM2INT(width);
        VALUE height = rb_hash_lookup(option, ID2SYM(rb_intern("height")));
        if (!NIL_P(height)) cHeight = NUM2INT(height);

        VALUE diff_x = rb_hash_lookup(option, ID2SYM(rb_intern("diff_x")));
        if (!NIL_P(diff_x)) cDiffX = NUM2INT(diff_x);
        VALUE diff_y = rb_hash_lookup(option, ID2SYM(rb_intern("diff_y")));
        if (!NIL_P(diff_y)) cDiffY = NUM2INT(diff_y);
    }
    if (cHeight == 0) cHeight = RARRAY_LENINT(map_ary);
    if (cWidth == 0) cWidth = RARRAY_LENINT(RARRAY_PTR(map_ary)[0]);

    VALUE* p_images = RARRAY_PTR(images);
    int tileWidth = NUM2INT(rb_funcall(p_images[0], rb_intern("width"), 0));
    int tileHeight = NUM2INT(rb_funcall(p_images[0], rb_intern("height"), 0));
    if (cDiffX != 0) cWidth++;
    if (cDiffY != 0) cHeight++;
    if (cDiffX < 0) { cDiffX+=tileWidth; cSrcX--; }
    if (cDiffY < 0) { cDiffY+=tileHeight; cSrcY--; }
    for (y = 0; y < cHeight; ++y) {
        VALUE* map_line = RARRAY_PTR(RARRAY_PTR(map_ary)[y + cSrcY]);
        for (x = 0; x < cWidth; ++x) {
            screen_draw(self, p_images[NUM2INT(map_line[x + cSrcX])], 
                rb_int_new(cDstX+x*tileWidth-cDiffX), rb_int_new(cDstY+y*tileHeight-cDiffY) );
        }
    }


    return Qnil;
}

static inline SDL_Texture* CreateTextTextureAtCache(ScreenData* screen, int* fontSize, TTF_Font* font, const char * text, SDL_Color* color, Uint8* alpha) {
    DrawTextCache *buff = screen->drawTextCache;
    SDL_Texture* texture = NULL;
    size_t result_length = strlen(text);
    size_t malloc_length = sizeof(char) * result_length;
    while (buff != NULL) {
        if (buff->size == *fontSize) {
            if ((strncmp(buff->text,  text, strlen(buff->text)) == 0)) {
                if (buff->color.r == (*color).r && buff->color.g == (*color).g &&
                    buff->color.b == (*color).b && buff->color.a == (*color).a) {
                    if (buff->font == font) { texture = buff->texture; break; }
                }
            }
        } 
        if (buff->next == NULL) {
            buff->next = malloc(sizeof(DrawTextCache));
            buff->next->font = font; buff->next->size = *fontSize;
            buff->next->color = *color;
            buff->next->text = malloc(malloc_length);
            snprintf(buff->next->text, result_length, text);
            SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, *color);
            texture = SDL_CreateTextureFromSurface(screen->renderer, surf);
            SDL_FreeSurface(surf);
            SDL_SetTextureAlphaMod(texture, *alpha);
            buff->next->texture = texture;
            buff->next->prev = buff;
            buff->next->next = NULL;

            int w,h; SDL_QueryTexture(texture, NULL, NULL, &w, &h);
            buff->next->textureByteSize = w * h * sizeof(uint32_t);
            screen->drawTextCacheSize += buff->next->textureByteSize;
            if (screen->drawTextCacheSize > DRAW_TEXT_CACHE) {
                const int cacheSize = 1024*1024*10;
                int totalCache = buff->next->textureByteSize;;
                buff = buff->next;
                while(buff->prev != NULL) {
                    buff = buff->prev;
                    totalCache += buff->textureByteSize;
                    if (totalCache > cacheSize) break;
                }
                DrawTextCache* temp = buff->next; 
                while (buff->size != 0) {
                    free(buff->text);
                    SDL_DestroyTexture(buff->texture);
                    buff = buff->prev;
                    free(buff->next);
                }
                screen->drawTextCache->next = temp;
                temp->prev = screen->drawTextCache;
            }
            break;
        }
        buff = buff->next;
    }
    return texture;
}

static VALUE screen_draw_text(int argc, VALUE argv[], VALUE self) {
    ScreenData *screen; Data_Get_Struct(self, ScreenData, screen);
    volatile VALUE rb_message, rb_x, rb_y, rb_font, rb_color, rb_alpha;
    rb_scan_args(argc, argv, "42", &rb_message, &rb_x, &rb_y, &rb_font, &rb_color, &rb_alpha);
    if (rb_color == Qnil) rb_color = rb_ary_new3(4, rb_int_new(255), rb_int_new(255), rb_int_new(255), rb_int_new(255));
    if (rb_alpha == Qnil) rb_alpha = rb_int_new(255);
    if (RARRAY_LEN(rb_color) == 3) rb_funcall(rb_color, rb_intern("push"), 1, rb_int_new(255));
    SDL_Color color;
    color.r = NUM2INT(RARRAY_PTR(rb_color)[0]);
    color.g = NUM2INT(RARRAY_PTR(rb_color)[1]);
    color.b = NUM2INT(RARRAY_PTR(rb_color)[2]);
    color.a = NUM2INT(RARRAY_PTR(rb_color)[3]);
    Uint8 alpha = NUM2INT(rb_alpha);
    const char*text = StringValuePtr(rb_message);
    int fontSize = rb_int_new(rb_funcall(rb_font, rb_intern("size"), 0));
    FontData *fontData; Data_Get_Struct(rb_font, FontData, fontData);
    TTF_Font *font = fontData->font;
    SDL_Texture * texture = CreateTextTextureAtCache(screen, &fontSize, font, text, &color, &alpha);

    SDL_SetRenderTarget(screen->renderer, screen->texture);
    SDL_Rect rect; rect.x = NUM2INT(rb_x); rect.y = NUM2INT(rb_y);
    SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
    SDL_RenderCopy(screen->renderer, texture, screen->rect, &rect);
    SDL_SetRenderTarget(screen->renderer, NULL);
    return self;
}

static VALUE module_color_get(int argc, VALUE argv[], VALUE self) {
    VALUE color, rest_colors;
    rb_scan_args(argc, argv, "1*", &color, &rest_colors);

    int len = RARRAY_LENINT(rest_colors);
    if (len == 0) {
        return rb_funcall(self, rb_intern("const_get"), 1, rb_funcall(color, rb_intern("upcase"), 0));
    } else {
        rb_ary_unshift(rest_colors, color); len++;
        volatile VALUE result = rb_ary_new2(len);
        int i; for (i = 0; i < len; ++i) {
            VALUE temp_color = RARRAY_PTR(rest_colors)[i];
            VALUE name = rb_funcall(self, rb_intern("const_get"), 1, rb_funcall(temp_color, rb_intern("upcase"), 0));
            rb_ary_push(result, name);
        }
        return result;
    }
}

static double round_d(double x)
{
    if ( x > 0.0f) {
        return SDL_floor(x + 0.5f);
    } else {
        return -1 * SDL_floor(SDL_fabs(x) + 0.5f);
    }
}


static VALUE module_color_hsl(int argc, VALUE argv[], VALUE self) {
    VALUE rb_h, rb_s, rb_l, rb_a, rb_ary;
    rb_scan_args(argc, argv, "13", &rb_h, &rb_s, &rb_l, &rb_a);
    float h, s, l, a;

    if (RB_TYPE_P(rb_h, T_ARRAY)) {
        rb_ary = rb_h;
        rb_h = rb_ary_entry(rb_ary, 0);
        rb_s = rb_ary_entry(rb_ary, 1);
        rb_l = rb_ary_entry(rb_ary, 2);
        rb_a = rb_ary_entry(rb_ary, 3);
    }

    a = NIL_P(rb_a) ? 1.0f : ( RB_TYPE_P(rb_a, T_FLOAT) ? NUM2DBL(rb_a) : NUM2DBL(rb_a)/255.0f);

    s = RB_TYPE_P(rb_s, T_FLOAT) ? NUM2DBL(rb_s) : NUM2DBL(rb_s)/100.0f;
    l = RB_TYPE_P(rb_l, T_FLOAT) ? NUM2DBL(rb_l) : NUM2DBL(rb_l)/100.0f;

    float r, g, b;
    r = g = b = 0.0f;
    if (s == 0) {
        r = g = b = l;
    } else  {
        if (RB_TYPE_P(rb_h, T_FLOAT)) {
            h = NUM2DBL(rb_h);
        } else {
            float temp = NUM2DBL(rb_h);
            while ( temp < 0) temp += 360.0f;
            h = temp / 60.0f;
        }
        int i = (int)SDL_floor(h);
        float f = h - i, c;

        if (l < 0.5f) {
            c = 2.0f * s * l;
        } else {
            c = 2.0f * s * (1.0f - l);
        }
        float m = l - c / 2.0f; 
        float p = c + m;        
        float q;
        if ( i % 2 == 0) {
            q = l + c * (f - 0.5f);
        } else {
            q = l - c * (f - 0.5f);
        }

        switch (i) {
            case 0:
                r = p; g = q; b = m;
                break;
            case 1:
                r = q; g = p; b = m;
                break;
            case 2:
                r = m; g = p; b = q;
                break;
            case 3:
                r = m; g = q; b = p;
                break;
            case 4:
                r = q; g = m; b = p;
                break;
            case 5:
                r = p; g = m; b = q;
                break;
        }
    }
    if (NIL_P(rb_a)) {
        return rb_ary_new3(3, rb_int_new(round_d(r*255.0f)), rb_int_new(round_d(g*255.0f)), rb_int_new(round_d(b*255.0f)));
    } else {
        return rb_ary_new3(4, rb_int_new(round_d(r*255.0f)), rb_int_new(round_d(g*255.0f)), rb_int_new(round_d(b*255.0f)), rb_int_new(round_d(a*255.0f)));
    }
} 

static VALUE module_color_rgb_to_hsl(int argc, VALUE argv[], VALUE self) {
    VALUE rb_r, rb_g, rb_b, rb_a, rb_ary;
    rb_scan_args(argc, argv, "13", &rb_r, &rb_g, &rb_b, &rb_a);
    if (RB_TYPE_P(rb_r, T_ARRAY)) {
        rb_ary = rb_r;
        rb_r = rb_ary_entry(rb_ary, 0);
        rb_g = rb_ary_entry(rb_ary, 1);
        rb_b = rb_ary_entry(rb_ary, 2);
        rb_a = rb_ary_entry(rb_ary, 3);
    }
    float r, g, b, a;
    r = RB_TYPE_P(rb_r, T_FLOAT) ? NUM2DBL(rb_r) : NUM2DBL(rb_r)/255.0f;
    g = RB_TYPE_P(rb_g, T_FLOAT) ? NUM2DBL(rb_g) : NUM2DBL(rb_g)/255.0f;
    b = RB_TYPE_P(rb_b, T_FLOAT) ? NUM2DBL(rb_b) : NUM2DBL(rb_b)/255.0f;
    a = NIL_P(rb_a) ? 1.0f : ( RB_TYPE_P(rb_a, T_FLOAT) ? NUM2DBL(rb_a) : NUM2INT(rb_a)/255.0f);

    float max_c = r,  min_c = r;
    float h, s, l;
    if (max_c < g) max_c = g;
    if (max_c < b) max_c = b;
    if (min_c > g) min_c = g;
    if (min_c > b) min_c = b;
    
    l = (max_c + min_c) / 2.0f;
    if (max_c == min_c) {
        h = s = 0.0f;
    } else {
        float c = max_c - min_c;
        if (max_c == r) {
            h = (g - b) / c;
        } else if (max_c == g) {
            h = (b - r) / c + 2.0f;
        } else {
            h = (r - g) / c + 4.0f;
        }
        h*=60.0f;
        
        if (h < 0.0f) h += 360.0f; 
        if (l < 0.5f) {
            s = c / (max_c + min_c);
        } else {
            s = c / (2.0f - max_c - min_c);
        }
    }
    if (NIL_P(rb_a)) {
        return rb_ary_new3(3, rb_int_new(round_d(h)), rb_int_new(round_d(s*100.0f)), rb_int_new(round_d(l*100.0f)));
    } else {
        return rb_ary_new3(3, rb_int_new(round_d(h)), rb_int_new(round_d(s*100.0f)), rb_int_new(round_d(l*100.0f)), rb_int_new(round_d(a*255.0f)));
    }
} 
    
void Init_screen(VALUE parent) {
    volatile VALUE color_module = rb_define_module_under(parent,"Color");
    rb_define_module_function(color_module, "[]", module_color_get, -1);
    rb_define_module_function(color_module, "hsl", module_color_hsl, -1);
    rb_define_module_function(color_module, "rgb_to_hsl", module_color_rgb_to_hsl, -1);
    rb_define_const(color_module, "BLANK", rb_ary_new3(4, rb_int_new(0), rb_int_new(0), rb_int_new(0), rb_int_new(0)));
    rb_define_const(color_module, "BLACK", rb_ary_new3(3, rb_int_new(0), rb_int_new(0), rb_int_new(0)));
    rb_define_const(color_module, "GRAY", rb_ary_new3(3, rb_int_new(128), rb_int_new(128), rb_int_new(128)));
    rb_define_const(color_module, "WHITE", rb_ary_new3(3, rb_int_new(255), rb_int_new(255), rb_int_new(255)));
    rb_define_const(color_module, "RED", rb_ary_new3(3, rb_int_new(255), rb_int_new(0), rb_int_new(0)));
    rb_define_const(color_module, "GREEN", rb_ary_new3(3, rb_int_new(0), rb_int_new(255), rb_int_new(0)));
    rb_define_const(color_module, "DARKGREEN", rb_ary_new3(3, rb_int_new(0), rb_int_new(64), rb_int_new(0)));
    rb_define_const(color_module, "LIGHTGREEN", rb_ary_new3(3, rb_int_new(90), rb_int_new(238), rb_int_new(90)));
    rb_define_const(color_module, "BLUE", rb_ary_new3(3, rb_int_new(0), rb_int_new(0), rb_int_new(255)));
    rb_define_const(color_module, "SKYBLUE", rb_ary_new3(3, rb_int_new(0), rb_int_new(255), rb_int_new(255)));
    rb_define_const(color_module, "YELLOW", rb_ary_new3(3, rb_int_new(255), rb_int_new(255), rb_int_new(0)));
    rb_define_const(color_module, "PURPLE", rb_ary_new3(3, rb_int_new(255), rb_int_new(0), rb_int_new(255)));
    rb_define_const(color_module, "ORANGE", rb_ary_new3(3, rb_int_new(255), rb_int_new(165), rb_int_new(0)));
    volatile VALUE screen_class= rb_define_class_under(parent, "Screen", rb_cObject);
    rb_define_alloc_func(screen_class, screen_alloc);
    rb_define_private_method(screen_class, "initialize", screen_initialize, -1);
    rb_define_method(screen_class, "[]", screen_get_pixel, 2);
    rb_define_method(screen_class, "[]=", screen_set_pixel, -1);
    rb_define_method(screen_class, "draw", screen_draw, 3);
    rb_define_method(screen_class, "draw_ex", screen_draw_ex, -1);
    rb_define_method(screen_class, "draw_actor", screen_draw_actor, 1);
    rb_define_method(screen_class, "draw_tiles", screen_draw_tiles, -1);
    rb_define_method(screen_class, "draw_text", screen_draw_text, -1);
    rb_define_method(screen_class, "x", screen_get_x, 0);
    rb_define_method(screen_class, "x=", screen_set_x, 1);
    rb_define_method(screen_class, "y", screen_get_y, 0);
    rb_define_method(screen_class, "y=", screen_set_y, 1);
    rb_define_method(screen_class, "blend", screen_get_blend, 0);
    rb_define_method(screen_class, "blend=", screen_set_blend, 1);
    rb_define_method(screen_class, "width", screen_get_width, 0);
    rb_define_method(screen_class, "height", screen_get_height, 0);
    rb_define_method(screen_class, "bgcolor", screen_bgcolor, 1);
    rb_define_method(screen_class, "to_img", screen_to_image, 0);
    rb_define_method(screen_class, "line", screen_draw_line, 4);
    rb_define_method(screen_class, "lines", screen_draw_lines, -1);
    rb_define_method(screen_class, "rect", screen_draw_rect, 4);
    rb_define_method(screen_class, "rects", screen_draw_rects, -1);
    rb_define_method(screen_class, "fill_rect", screen_draw_fill_rect, 4);
    rb_define_method(screen_class, "fill_rects", screen_draw_fill_rects, -1);


}
#include "sdl_udon_private.h"

SDL_Surface* TextureToSurface(SDL_Texture* texture, Uint32 * colorKey) {
    SDL_Surface* surface;
    void *pixels; int pitch;
    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    uint32_t *surf_pixels = malloc(sizeof(uint32_t) * w * h);
    memcpy(surf_pixels, (uint32_t*)pixels, sizeof(uint32_t) * w * h);
    surface = SDL_CreateRGBSurfaceFrom(surf_pixels, w, h, 32, pitch,0,0,0,0);
    SDL_UnlockTexture(texture);

    Uint8 r, g, b, a;
    SDL_BlendMode blendMode;
    SDL_GetTextureColorMod(texture, &r, &g, &b);
    SDL_SetSurfaceColorMod(surface, r, g, b);
    SDL_GetTextureAlphaMod(texture, &a);
    SDL_SetSurfaceAlphaMod(surface, a);
    if (*colorKey != 0) {
        SDL_SetColorKey(surface, SDL_TRUE , *colorKey);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    } else {
        SDL_GetTextureBlendMode(texture, &blendMode);
        SDL_SetSurfaceBlendMode(surface, blendMode);
    }
    return surface;
}

SDL_Texture* CreateTexture(int width, int height, SDL_Renderer* ren) {
    SDL_Texture * texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
       SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
    return texture;
}

SDL_Texture* CreateTextureFromSurfacePixels(SDL_Surface* surface, SDL_Renderer* ren, SDL_Color * outColorKey) {
    SDL_Texture* texture;
    const SDL_PixelFormat *fmt;
    SDL_bool needAlpha;
    Uint32 i, format;
    fmt = surface->format;
    needAlpha = (fmt->Amask || SDL_GetColorKey(surface, NULL) == 0) ? SDL_TRUE : SDL_FALSE;
    SDL_RendererInfo info; SDL_GetRendererInfo(ren, &info);    
    format = info.texture_formats[0];
    for (i = 0; i < info.num_texture_formats; ++i) {
        if (!SDL_ISPIXELFORMAT_FOURCC(info.texture_formats[i]) &&
            SDL_ISPIXELFORMAT_ALPHA(info.texture_formats[i]) == needAlpha) {
            format = info.texture_formats[i];
            break;
        }
    }
    texture = SDL_CreateTexture(ren, format, SDL_TEXTUREACCESS_STREAMING, surface->w, surface->h);
    SDL_PixelFormat *dst_fmt = SDL_AllocFormat(format);
    SDL_Surface *temp = SDL_ConvertSurface(surface, dst_fmt, 0);
    SDL_FreeFormat(dst_fmt);
    if (temp) {
        SDL_UpdateTexture(texture, NULL, temp->pixels, temp->pitch);
        SDL_FreeSurface(temp);
    }
    Uint8 r, g, b, a;
    SDL_BlendMode blendMode;
    SDL_GetSurfaceColorMod(surface, &r, &g, &b);
    SDL_SetTextureColorMod(texture, r, g, b);
    SDL_GetSurfaceAlphaMod(surface, &a);
    SDL_SetTextureAlphaMod(texture, a);

    Uint32 colorKey = 0;
    if (SDL_GetColorKey(surface, &colorKey) == 0) {
        *outColorKey = surface->format->palette->colors[colorKey];
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    } else {
        SDL_GetSurfaceBlendMode(surface, &blendMode);
        SDL_SetTextureBlendMode(texture, blendMode);
    }
    return texture;
}
void CreateTextureAndSetImageData(const char* filename, SDL_Renderer* ren, ImageData *image) {
    SDL_Surface* surface = IMG_Load(filename);
    if (surface == NULL) { SDL_LOG_ABORT(); }
    image->texture = CreateTextureFromSurfacePixels(surface, ren, &image->colorKey);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(image->texture, NULL, NULL, &image->rect->w, &image->rect->h);
}

SDL_Texture* loadTexture(const char* filename, SDL_Renderer *ren) {
    SDL_Texture *texture = IMG_LoadTexture(ren, filename);
    if (texture == NULL) { SDL_LOG_ABORT(); }
    return texture;
}

inline void renderTexture(SDL_Texture *tex, SDL_Renderer *ren, SDL_Rect *dst) { SDL_RenderCopy(ren, tex, NULL, dst); }

void renderTextureRect(SDL_Texture *tex, SDL_Renderer *ren, int x, int y, int x2, int y2, int w2, int h2) {
    SDL_Rect clip = {x2, y2, w2, h2};
    SDL_Rect dst;
    dst.x = x; dst.y = y; dst.w = clip.w; dst.h = clip.h;
    SDL_RenderCopy(ren,tex, &clip, &dst);
}

SDL_Texture* renderText(const char* str, const char* fontName, SDL_Color color, int  fontSize, SDL_Renderer *ren) {
    TTF_Font *font = TTF_OpenFont(fontName, fontSize);
    if (font == NULL) SDL_LOG_ABORT();

    SDL_Surface *surf = TTF_RenderText_Blended(font, str, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FreeSurface(surf);
    TTF_CloseFont(font);
    return texture;
}




static void image_dfree(ImageData *image)  {
    SDL_free(image->rect);
    if ((image->attach_id == 1) && systemData->gameRunning)  {
        SDL_DestroyTexture(image->texture);
    }
    free(image);
}

static VALUE image_alloc(VALUE klass) {
    ImageData *image = ALLOC(ImageData);
    return Data_Wrap_Struct(klass, 0, image_dfree, image);
}


static VALUE image_initialize(int argc, VALUE argv[], VALUE self) {
    volatile VALUE width, height, color, window_id;
    SDL_Renderer* renderer;
    rb_scan_args(argc, argv, "22", &width, &height, &color, &window_id);
    if (NIL_P(window_id)) {
        window_id = rb_int_new(0);
        renderer = systemData->renderer;
    } else {
        renderer = GetWindowRenderer(&self, NUM2INT(window_id));
        if (renderer == NULL) { SDL_DETAILLOG_ABORT("Window NotFound!"); }
    }
    ImageData* image; Data_Get_Struct(self, ImageData, image);
    image->colorKey.a = 0;image->colorKey.r = 0;image->colorKey.g = 0;image->colorKey.b = 0;
    image->attach_id = NUM2INT(window_id) + 1;
    image->rect = SDL_malloc(sizeof(SDL_Rect));
    image->rect->x = image->rect->y = 0;
    image->texture = CreateTexture(NUM2INT(width), NUM2INT(height), renderer);
    SDL_QueryTexture(image->texture, NULL, NULL, &image->rect->w, &image->rect->h);
    if (!NIL_P(color)) { rb_funcall(self, rb_intern("fill"), 1, color); }
    return Qnil;
}

static VALUE image_load(int argc, VALUE argv[], VALUE self) {
    volatile VALUE filename, window_id;
    SDL_Renderer* renderer;
    if (rb_scan_args(argc, argv, "11", &filename, &window_id) == 1) window_id = rb_int_new(0);
    ImageData *image = NULL;
    volatile VALUE image_instance = Qundef;
    VALUE ext_name = rb_funcall(rb_cFile, rb_intern("extname"), 1, rb_funcall(filename, rb_intern("to_s"), 0));
    if (rb_funcall(ext_name, rb_intern("empty?"), 0) == Qtrue ) {
        VALUE game = rb_path2class("SDLUdon::Game");
        image_instance = rb_funcall(game, rb_intern("fetch"), 3, rb_str_new2("image"), filename, window_id);
        if (NIL_P(image_instance)) rb_raise(rb_eLoadError, "%s", "image not found...!");
        Data_Get_Struct(image_instance, ImageData, image);
    } else {
        renderer = NIL_P(window_id) ? systemData->renderer : GetWindowRenderer(&self, NUM2INT(window_id));
        image_instance = rb_obj_alloc(self); 
        Data_Get_Struct(image_instance, ImageData, image);
        image->colorKey.a = 0;image->colorKey.r = 0;image->colorKey.g = 0;image->colorKey.b = 0;
        image->rect = SDL_malloc(sizeof(SDL_Rect));
        CreateTextureAndSetImageData(StringValuePtr(filename), renderer, image);
    }
    image->rect->x = image->rect->y = 0;
    image->attach_id = NUM2INT(window_id) + 1;
    return image_instance;
}

static VALUE image_class_text(int argc, VALUE argv[], VALUE self) {
    VALUE text; volatile VALUE opt;
    if (rb_scan_args(argc, argv, "11", &text, &opt) == 1) { opt = rb_hash_new(); }

    volatile VALUE rb_x = rb_hash_lookup(opt, ID2SYM(rb_intern("x")));
    if (NIL_P(rb_x)) rb_x = rb_int_new(0);
    volatile VALUE rb_y = rb_hash_lookup(opt, ID2SYM(rb_intern("y")));
    if (NIL_P(rb_y)) rb_y = rb_int_new(0);
    volatile VALUE rb_font = rb_hash_lookup(opt, ID2SYM(rb_intern("font")));
    if (NIL_P(rb_font)) rb_font = rb_class_new_instance(2, (VALUE[]){ rb_str_new2("ＭＳ ゴシック"), rb_int_new(10) }, rb_path2class("SDLUdon::Font"));
    volatile VALUE rb_color = rb_hash_lookup(opt, ID2SYM(rb_intern("color")));
    if (NIL_P(rb_color)) rb_color = rb_ary_new3(3, rb_int_new(255),rb_int_new(255),rb_int_new(255));
    volatile VALUE rb_window_id = rb_hash_lookup(opt, ID2SYM(rb_intern("window_id")));
    if (NIL_P(rb_window_id)) rb_window_id = rb_int_new(0);

    VALUE rb_height = rb_funcall(rb_font, rb_intern("size"), 0);
    int cHeight = NUM2INT(rb_height);
    volatile VALUE text_ary = rb_str_split(text,"\n");
    int text_ary_size = RARRAY_LENINT(text_ary);
    int i, max_w = 0;
    
    for (i = 0; i < text_ary_size; ++i) {
        int w = NUM2INT(rb_funcall(rb_font, rb_intern("text_width"), 1, RARRAY_PTR(text_ary)[i]));
        if (w > max_w) max_w = w;
    }
    volatile VALUE image_instance = rb_class_new_instance(3, 
        (VALUE[]){ rb_int_new(max_w), rb_int_new(cHeight*text_ary_size), Qnil, rb_window_id }, self);

    rb_funcall(image_instance, rb_intern("text"), 5, text, rb_x, rb_y, rb_font, rb_color);
    return image_instance;
}

static VALUE image_dispose(VALUE self) {
    ImageData *image; Data_Get_Struct(self, ImageData, image);
    SDL_DestroyTexture(image->texture);
    image->texture = NULL;
    return self;
}

static VALUE image_save(VALUE self, VALUE filename) {
    volatile VALUE ext_name = rb_funcall(rb_cFile, rb_intern("extname"), 1, filename);
    const char* cExtName = StringValuePtr(ext_name);
    ImageData *image;Data_Get_Struct(self, ImageData, image);
    void *pixels; int pitch;
    SDL_LockTexture(image->texture, NULL, &pixels, &pitch);
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixels, image->rect->w, image->rect->h,
        32, pitch,0,0,0,0);
    SDL_UnlockTexture(image->texture);

    int len = strlen(cExtName);
    if (len == 0) {
        VALUE base_name = rb_funcall(rb_cFile, rb_intern("basename"), 2, filename, ext_name);
        VALUE str = rb_str_cat2(base_name, ".png");
        IMG_SavePNG(surface, StringValuePtr(str));
    } else if (strncmp(cExtName, ".png", len) == 0) {
        IMG_SavePNG(surface, StringValuePtr(filename));
    } else if (strncmp(cExtName, ".bmp", len) == 0) {
        SDL_SaveBMP(surface, StringValuePtr(filename));
    } else {
        VALUE base_name = rb_funcall(rb_cFile, rb_intern("basename"), 2, filename, ext_name);
        VALUE str = rb_str_cat2(base_name, ".png");
        IMG_SavePNG(surface, StringValuePtr(str));
    }
    SDL_FreeSurface(surface);
    return self;
}

static VALUE image_scan(VALUE self, VALUE rb_x, VALUE rb_y, VALUE rb_width, VALUE rb_height) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    int width = NUM2INT(rb_width);  int height = NUM2INT(rb_height); 
    int x = NUM2INT(rb_x); int y = NUM2INT(rb_y);
    if ((width - x) > imageData->rect->w) width = imageData->rect->w;
    if ((height - y) > imageData->rect->h) height = imageData->rect->h;
    volatile VALUE result_image = rb_class_new_instance(3, (VALUE[]){rb_int_new(width), rb_int_new(height), Qnil, rb_int_new(imageData->attach_id) }, rb_class_of(self));
    rb_funcall(result_image, rb_intern("draw"), 7, self, rb_int_new(0), rb_int_new(0),
        rb_x, rb_y, rb_width, rb_height);
    return result_image;
}

static VALUE image_scan_tiles(int argc, VALUE argv[], VALUE self) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    VALUE rb_cutx, rb_cuty, rb_x, rb_y;
    rb_scan_args(argc, argv, "22", &rb_cutx, &rb_cuty, &rb_x, &rb_y);
    if (NIL_P(rb_x)) rb_x = rb_int_new(0); if (NIL_P(rb_y)) rb_y = rb_int_new(0);
    int cutx = NUM2INT(rb_cutx); int cuty = NUM2INT(rb_cuty);
    int x = NUM2INT(rb_x); int y = NUM2INT(rb_y);
    int width = (imageData->rect->w - x) / cutx; 
    int height = (imageData->rect->h - y) / cuty;
    volatile VALUE result_ary = rb_ary_new2(cutx * cuty);
    int i, j;
    for (i = 0; i < cuty; ++i) {
        for (j = 0; j < cutx; ++j) {
            VALUE scan_args[4] = { rb_int_new(x + j*width), rb_int_new(y + i*height), rb_int_new(width), rb_int_new(height) };
            volatile VALUE image = rb_funcall2(self, rb_intern("scan"), 4, scan_args);
            rb_ary_push(result_ary, image);
        }
    }
    return result_ary;

}

static VALUE image_draw(int argc, VALUE argv[], VALUE self) {
    VALUE image, rb_x, rb_y, rb_sx, rb_sy, rb_width, rb_height;
    rb_scan_args(argc, argv, "34", &image, &rb_x, &rb_y, &rb_sx, &rb_sy, &rb_width, &rb_height);
    int x = NUM2INT(rb_x), y = NUM2INT(rb_y);
    ImageData* dstData; Data_Get_Struct(self, ImageData, dstData);
    ImageData* srcData; Data_Get_Struct(image, ImageData, srcData);
    int srcWidth = (NIL_P(rb_width)) ? srcData->rect->w : NUM2INT(rb_width); 
    int srcHeight = (NIL_P(rb_height)) ? srcData->rect->h : NUM2INT(rb_height);
    int srcX = (NIL_P(rb_sx)) ? 0 : NUM2INT(rb_sx);
    int srcY = (NIL_P(rb_sy)) ? 0 : NUM2INT(rb_sy);
    void *pixels, *srcPixels; 
    int pitch, srcPitch;
    SDL_LockTexture(dstData->texture, dstData->rect, &pixels, &pitch);
    SDL_LockTexture(srcData->texture, srcData->rect, &srcPixels, &srcPitch);
    Uint32 *buff = pixels, *srcBuff = srcPixels, buffPixelFormat;
    SDL_QueryTexture(srcData->texture, &buffPixelFormat, NULL, NULL, NULL);
    SDL_PixelFormat *srcFmt = SDL_AllocFormat(buffPixelFormat);
    Uint32 srcColorKey = SDL_MapRGBA(srcFmt, srcData->colorKey.r, srcData->colorKey.g, srcData->colorKey.b, srcData->colorKey.a);
    SDL_FreeFormat(srcFmt);

    
    int i, j, diff_w = 0, diff_h = 0; Uint32 color;
    if (srcX + srcWidth > srcData->rect->w)
        srcWidth -= ( (srcX + srcWidth) - srcData->rect->w );
    if (srcY + srcHeight > srcData->rect->h)
        srcHeight -= ( (srcY + srcHeight) - srcData->rect->h );
    if (srcX < 0) srcX = 0;
    if (srcY < 0) srcY = 0;
    
    diff_w = x + srcWidth; diff_h = y + srcHeight;
    if (diff_w > dstData->rect->w) diff_w = dstData->rect->w; 
    if (diff_h > dstData->rect->w) diff_h = dstData->rect->h;
    if (x < 0) x = 0;if (y < 0) y = 0;

    for (i = y; i < diff_h; ++i) {
        for (j = x; j < diff_w; ++j) {
            color = srcBuff[(srcY + i - y) * srcData->rect->w + (srcX + j - x) ];
            if (color != srcColorKey) buff[i * dstData->rect->w + j] = color;
        }
    }

    SDL_UnlockTexture(srcData->texture);
    SDL_UnlockTexture(dstData->texture);

    return self;
}
static VALUE image_draw_copy(int argc, VALUE argv[], VALUE self) {
    VALUE image, rb_x, rb_y, rb_sx, rb_sy, rb_width, rb_height;
    rb_scan_args(argc, argv, "34", &image, &rb_x, &rb_y, &rb_sx, &rb_sy, &rb_width, &rb_height);
    int x = NUM2INT(rb_x), y = NUM2INT(rb_y);
    ImageData* dstData; Data_Get_Struct(self, ImageData, dstData);
    ImageData* srcData; Data_Get_Struct(image, ImageData, srcData);
    int srcWidth = (NIL_P(rb_width)) ? srcData->rect->w : NUM2INT(rb_width); 
    int srcHeight = (NIL_P(rb_height)) ? srcData->rect->h : NUM2INT(rb_height); 
    int srcX = (NIL_P(rb_sx)) ? 0 : NUM2INT(rb_sx);
    int srcY = (NIL_P(rb_sy)) ? 0 : NUM2INT(rb_sy);
    void *pixels, *srcPixels; 
    int pitch, srcPitch;
    SDL_LockTexture(dstData->texture, dstData->rect, &pixels, &pitch);
    SDL_LockTexture(srcData->texture, srcData->rect, &srcPixels, &srcPitch);
    Uint32 *buff = pixels, *srcBuff = srcPixels;

    int i, j, diff_w = 0, diff_h = 0; Uint32 color;
    if (srcX + srcWidth > srcData->rect->w) {
        srcWidth = srcWidth - ( (srcX + srcWidth) - srcData->rect->w );
    }
    if (srcY + srcHeight > srcData->rect->h) {
        srcHeight = srcHeight - ( (srcY + srcHeight) - srcData->rect->h );
    }
    if (srcX < 0) srcX = 0; if (srcX >= srcData->rect->w) srcX = srcData->rect->w-1;
    if (srcY < 0) srcY = 0; if (srcY >= srcData->rect->h) srcY = srcData->rect->h-1;

    diff_w = x + srcWidth; diff_h = y + srcHeight;
    if (diff_w > dstData->rect->w) diff_w = dstData->rect->w; 
    if (diff_h > dstData->rect->w) diff_w = dstData->rect->h;
    if ((diff_h - y) > srcHeight) diff_h = y + srcHeight;
    if ((diff_w - x) > srcWidth) diff_w = x + srcWidth;

    if (x < 0) x = 0;if (y < 0) y = 0;
    for (i = y; i < diff_h; ++i) {
        for (j = x; j < diff_w; ++j) {
            color = srcBuff[(srcY + i - y) * srcData->rect->w + (srcX + j - x) ];
            buff[i * srcData->rect->w + j] = color;
        }
    }
    SDL_UnlockTexture(srcData->texture);
    SDL_UnlockTexture(dstData->texture);
    return self;
}

static VALUE image_draw_point(VALUE self, VALUE x, VALUE y, VALUE color) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    void *pixels; int pitch;
    SDL_LockTexture(imageData->texture, NULL, &pixels, &pitch);
    Uint8 r = NUM2INT(rb_ary_entry(color, 0)); 
    Uint8 g = NUM2INT(rb_ary_entry(color, 1)); 
    Uint8 b= NUM2INT(rb_ary_entry(color, 2));
    Uint8 a = ( RARRAY_LENINT(color) == 4 ? NUM2INT(rb_ary_entry(color, 3)) : 255);
    SDL_PixelFormat * fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    Uint32 c_color = SDL_MapRGBA(fmt, r, g, b, a);
    SDL_FreeFormat(fmt);
    Uint32 *buff = pixels;
    buff[NUM2INT(y) * imageData->rect->w + NUM2INT(x)] = c_color;
    SDL_UnlockTexture(imageData->texture);
    return Qnil;
}


static int adjust_draw_line_scope(VALUE self, int *p_start, int *p_end, int sx, int sy, int ex, int ey, int *p_cx, int *p_cy) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    int img_lim, img_lim_other, base_lim, other_lim;
    int *p_res, base_co, other_co, tmp, cx, cy;
    cx = *p_cx = ex - sx;
    cy = *p_cy = ey - sy;
    int base_axis = (abs(cx) > abs(cy)) ? 'x' : 'y';
    *p_start = *p_end = 0;
    if (!cx && !cy) return -1;
    if (base_axis == 'x') {
        img_lim = imageData->rect->w; img_lim_other = imageData->rect->h;
        base_lim = cx; other_lim = cy;
    } else {
        img_lim = imageData->rect->h; img_lim_other = imageData->rect->w;
        base_lim = cy; other_lim = cx;
    }
    int i; for (i = 0 ; i < 2; ++i) {
        if (i == 0) {
            p_res = p_start;
            base_co = (base_axis == 'x') ? sx : sy;
            other_co = (base_axis == 'x') ? sy : sx;
        } else {
            p_res = p_end;
            base_co = (base_axis == 'x') ? ex : ey;
            other_co = (base_axis == 'x') ? ey : ex;
            other_lim = -other_lim;
        }
        if (base_co < 0) *p_res = -base_co;
        if (base_co >= img_lim) *p_res = base_co - img_lim + 1;
        if (!other_lim) continue;
        tmp = other_co + ((*p_res) * (2 * other_lim) + abs(base_lim))/(2*base_lim);

        int aim_co;
        if (tmp < 0) {
            aim_co = -other_co;
        } else if (tmp >= img_lim_other) {
            aim_co = other_co - img_lim_other + 1;
        } else {
            continue; 
        }

        tmp = aim_co * 2 * abs(base_lim) - abs(base_lim);
        *p_res = tmp / abs(2 * other_lim);
        if ( i == 0 && tmp % abs(2 * other_lim)) (*p_res)++;
        if ( i == 1) (*p_res)++;
    }
    *p_end = abs(base_lim) - *p_end;
    return 0;
}

static VALUE image_draw_line(VALUE self, VALUE v_sx, VALUE v_sy, VALUE v_ex, VALUE v_ey, VALUE color) {
    int sx = NUM2INT(v_sx), sy = NUM2INT(v_sy);
    int ex = NUM2INT(v_ex), ey = NUM2INT(v_ey);
    int i,col_index,lim,start,end; 
    i = col_index = lim = start = end = 0;
    int cx,cy,pos_x,pos_y;
    cx = cy = pos_x = pos_y = 0;
    double_t e;
    int *p_co1, *p_co2, ddis1, ddis2, dn1, dn2;
    if (adjust_draw_line_scope(self, &start, &end, sx, sy, ex, ey, &cx, &cy ) < 0) return Qnil;
    int base_axis = (abs(cx) > abs(cy)) ? 'x' : 'y';
    if (base_axis == 'x') {
        p_co1 = &pos_x; p_co2 = &pos_y;
        ddis1 = 2*cx; ddis2 = 2*cy;
    } else {
        p_co1 = &pos_y; p_co2 = &pos_x;
        ddis1 = 2*cy; ddis2 = 2*cx;
    }
    dn1 = dn2 = 1;
    if (ddis1<0) {
        dn1 = -1; ddis1 = -ddis1;
    }
    if (ddis2<0) {
        dn2 = -1; ddis2 = -ddis2;
    }
    pos_x = sx ; pos_y = sy;
    *p_co1 += dn1 * start;
    *p_co2 += dn2 * (start * ddis2 + ddis1 / 2) / ddis1;
    e = (ddis1 / 2 + start * ddis2) % ddis1;

    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    SDL_Texture* texture = imageData->texture;
    void *pixels; int pitch;
    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    Uint8 r = rb_ary_entry(color, 0); Uint8 g = rb_ary_entry(color, 1); Uint8 b= rb_ary_entry(color, 2);
    SDL_PixelFormat * fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    Uint32 c_color = SDL_MapRGBA(fmt, r, g, b, 255);
    SDL_FreeFormat(fmt);
    Uint32 *buff = pixels;

    for (i = start; i <= end; ++i) {
        buff[pos_y * imageData->rect->w + pos_x] = c_color;
        (*p_co1) += dn1;
        e += ddis2;
        if (e >= ddis1) {
            e -= ddis1;
            (*p_co2) += dn2;
        }
    }
    SDL_UnlockTexture(texture);
    return self;
}

static double_t root(double_t x) {
    double_t s = 1, s2 = 1;
    if (x <= 0) return 1;
    do { s2 =s; s = (x / s + s) / 2; } while (s2 != s);
    return s;
}
static int root_i(int x) {
    int s = 1, s2 = 1;
    if (x <= 0) return 1;
    do {
        s2 = s;  
        s = ( x / s + s)/ 2;
        s2 = s + 1;
        if (s * s <= x && x < s2 * s2) break;
    } while (1);
    return s;
}




struct _pointArray;

typedef struct _pointArray {
    SDL_Point *pt;
    struct _pointArray *next;
} PointArray;


static void ScanLineO3(Uint32* pixelsBuff, int width, int leftX, int rightX, 
    int y, Uint32 startColor, SDL_Point **points, int *index) {
    while (leftX <= rightX) {
        for (; leftX <= rightX; leftX++) {
            if (pixelsBuff[width*y+leftX] == startColor) break;
        }
        if (rightX < leftX) break;
        for (; leftX <= rightX; leftX++) {
            if (pixelsBuff[width*y+leftX] != startColor ) break;
        }
        points[*index]->x = leftX - 1; points[*index]->y = y;
        (*index)++;
    }
}

static void ScanLine(Uint32* pixels, int width, int leftX, int rightX, 
    int y, Uint32 startColor, PointArray* ptArray) {
    PointArray* ptAryBuff;
    while (leftX <= rightX) {
        for (; leftX <= rightX; leftX++) {
            if (pixels[width*y+leftX] == startColor) break;
        }
        if (rightX < leftX) break;
        for (; leftX <= rightX; leftX++) {
            if (pixels[width*y+leftX] != startColor ) break;
        }
        ptAryBuff = ptArray;
        while (ptAryBuff->next != NULL) ptAryBuff = ptAryBuff->next;
        ptAryBuff->next = SDL_malloc(sizeof(PointArray));
        ptAryBuff->next->next = NULL;
        ptAryBuff->next->pt = SDL_malloc(sizeof(SDL_Point));
        ptAryBuff->next->pt->x = leftX - 1;
        ptAryBuff->next->pt->y = y;
    }
}

static inline void FillShapeScanLineO3(Uint32 *pixels, int x, int y, int width, int height, 
    Uint32 fillColor, Uint32 startColor) {
    if ( startColor == fillColor ) return;
    SDL_Point pt;
    int i, leftX, rightX;
    i = leftX = rightX = 0;
    int now_i = 0, diff_i = 1;
    int size = (height/2) * (width/3);
    SDL_Point **points = (SDL_Point**)calloc(size, sizeof(SDL_Point*));
    for (i = 0; i < size; ++i) {
        points[i] = malloc(sizeof(SDL_Point));
        points[i]->x = -1;
    }
    points[0]->x = x; points[0]->y = y;

    while (TRUE) {
        pt.x = -1;
        for (i = now_i; i < diff_i; i++) {
            if (points[i]->x != -1) {
                pt.x = points[i]->x; pt.y = points[i]->y;
                points[i]->x = -1;
                break;
            }
        }
        if (pt.x == -1) break;
        now_i++;
        if (pixels[width*pt.y+pt.x] == fillColor ) continue;
        for (leftX = pt.x; 0 <= leftX; leftX--) {
            if (pixels[width * pt.y + leftX -1] != startColor) break;
        }
        for (rightX = pt.x; rightX < width; rightX++) {
            if (pixels[width * pt.y + rightX + 1] != startColor) break;
        }
        for (i = leftX; i <= rightX; i++) pixels[width*pt.y+i] = fillColor;
        if (pt.y + 1 < height) ScanLineO3(pixels, width, leftX, rightX, pt.y + 1, startColor, points, &diff_i);
        if (0 <= pt.y - 1) ScanLineO3(pixels, width, leftX, rightX, pt.y - 1, startColor, points, &diff_i);
    }
    for (i = 0; i < size; ++i) free(points[i]);
    free(points);
} 

static inline void FillShapeScanLine(Uint32 *pixels, int x, int y, int width, int height, 
    Uint32 fillColor, Uint32 startColor) 
{
    if ( startColor == fillColor ) return;
    PointArray *ptAry = malloc(sizeof(PointArray));
    ptAry->pt = malloc(sizeof(SDL_Point));
    ptAry->pt->x = ptAry->pt->y = -1;
    
    ptAry->next = malloc(sizeof(PointArray));
    ptAry->next->pt = malloc(sizeof(SDL_Point));
    ptAry->next->pt->x = x; ptAry->next->pt->y = y;
    ptAry->next->next = NULL;

    SDL_Point pt;
    int i, leftX, rightX;

    PointArray* ptAryBuff = ptAry;
    while (ptAryBuff->next != NULL) {
        ptAryBuff = ptAryBuff->next;
        pt.x = ptAryBuff->pt->x; pt.y = ptAryBuff->pt->y;

        if (pixels[width*pt.y+pt.x] == fillColor ) { continue; }
        for (leftX = pt.x; 0 <= leftX; leftX--) {
            if (pixels[width * pt.y +  leftX -1] != startColor) break;
        }
        for (rightX = pt.x; rightX < width; rightX++) {
            if (pixels[width*pt.y + rightX + 1] != startColor) break;
        }
        for (i = leftX; i <= rightX; i++) pixels[width*pt.y+i] = fillColor;
        if (pt.y + 1 < height) ScanLine(pixels, width, leftX, rightX, pt.y + 1, startColor, ptAryBuff);
        if (0 <= pt.y - 1) ScanLine(pixels, width, leftX, rightX, pt.y - 1, startColor, ptAryBuff);
    }

    ptAryBuff = ptAry;
    while (ptAryBuff) {
        PointArray* buff = ptAryBuff->next;
        free(ptAryBuff->pt);
        free(ptAryBuff);
        ptAryBuff = buff;
    }
}





static VALUE image_draw_circle(VALUE self, VALUE v_cx, VALUE v_cy, VALUE v_radius, VALUE color) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    int center_x = NUM2INT(v_cx) - 1;
    int center_y = NUM2INT(v_cy) - 1;
    int diameter = NUM2INT(v_radius) * 2;
    SDL_Point center;
    center.x = center_x; center.y = center_y;

    long cx = 0, cy = diameter/2, *px, *py, tmp;
    long dy, dx, x_sign, y_sign, num_eight, r_root2;
    double_t d;
    r_root2 = (diameter > 3) ? root(diameter*diameter/8) : 1;
    tmp = r_root2 * r_root2 * 8 - diameter * diameter;
    if (abs(tmp) > abs(tmp + 8 * ( 2 * r_root2 + 1))) r_root2++; 

    d = -diameter * diameter + 4 * cy * cy - 4 * cy + 2;
    dx = 4;
    dy = -8 * cy + 8;

    SDL_Point mirror_center = center, now_center, start_po[8], end_po[8];
    if ((diameter & 1) == 0) {
        mirror_center.x++;
        mirror_center.y++;
    }
    for (num_eight = 0; num_eight < 8; num_eight++) {
        start_po[num_eight].y = diameter / 2;
        start_po[num_eight].x = 0;
        end_po[num_eight].x = end_po[num_eight].y = r_root2;
    }

    int li = 0; for (li = 0; li < 4; li++) {
        if (li == 0) {
            cy = center.y - ( imageData->rect->h - 1); y_sign = -1;
        }
        if (li == 1) {
            cy = mirror_center.x; y_sign = 1;
        }
        if (li == 2) {
            cy = mirror_center.y; y_sign = 1;
        }
        if (li == 3) {
            cy = center.x - (imageData->rect->w - 1); y_sign = -1;
        }
        if (abs(cy) >= (diameter/2)) {
            if (((li == 0 || li == 3) && cy < 0) || ((li == 1 || li == 2) && cy > 0)) {
                continue; 
            } else {
                return -1; 
            }
        }

        tmp = diameter * diameter - 4 * cy * cy;
        cx = root_i(tmp / 4); 
        tmp -= 4 * cx * cx;
        if (abs(tmp) >= abs(tmp - 8 * cx - 4)) cx++;
        if (cy * y_sign > r_root2) {
            if (start_po[li * 2 + 1].x < abs(cx)) {
                start_po[li * 2 + 1].y = abs(cy);
                start_po[li * 2 + 1].x = abs(cx);
            }
            if (start_po[(li * 2 + 2)%8].x < abs(cx)) {
                start_po[(li * 2 + 2)%8].y = abs(cy);
                start_po[(li * 2 + 2)%8].x = abs(cx);
            }
        } else {
            start_po[li * 2 + 1].y = start_po[(li * 2 + 2) % 8].y = 0; 
            start_po[li * 2 + 1].x = start_po[(li * 2 + 2) % 8].x = diameter; 
            if (cy * y_sign <= r_root2 && cy * y_sign > 0) {
                if (end_po[li * 2].x > abs(cy)) {
                    end_po[li * 2].y = abs(cx);
                    end_po[li * 2].x = abs(cy);
                }
                if (end_po[(li * 2 + 3) % 8].x > abs(cy)) {
                    end_po[(li * 2 + 3) % 8].y = abs(cx);
                    end_po[(li * 2 + 3) % 8].x = abs(cy);
                }
            } else {
                start_po[li * 2].y = start_po[(li * 2 + 3) % 8].y = 0;
                start_po[li * 2].x = start_po[(li * 2 + 3) % 8].x = diameter;
                if (cy * y_sign <= 0 && cy * y_sign > -r_root2) {
                    if (start_po[(li * 2 + 4) % 8].x < abs(cy)) {
                        start_po[(li * 2 + 4) % 8].y = abs(cx);
                        start_po[(li * 2 + 4) % 8].x = abs(cy);
                    }
                    if (start_po[(li * 2 + 7) % 8].x < abs(cy)) {
                        start_po[(li * 2 + 7) % 8].y = abs(cx);
                        start_po[(li * 2 + 7) % 8].x = abs(cy);
                    }
                } else {
                    start_po[(li * 2 + 4) % 8].y = start_po[(li * 2 + 7) % 8].y = 0;
                    start_po[(li * 2 + 4) % 8].x = start_po[(li * 2 + 7) % 8].x = diameter;
                    if (end_po[(li * 2 + 5) % 8].x > abs(cx)) {
                        end_po[(li * 2 + 5) % 8].y = abs(cy);
                        end_po[(li * 2 + 5) % 8].x = abs(cx);
                    }
                    if (end_po[(li * 2 + 6) % 8].x > abs(cx)) {
                        end_po[(li * 2 + 6) % 8].y = abs(cy);
                        end_po[(li * 2 + 6) % 8].x = abs(cx);
                    }
                }
            }
        }
    }
    SDL_Texture* texture = imageData->texture;
    void *pixels; int pitch;
    Uint8 a, r, g, b;
    Uint32 c_color, *buff;

    for (num_eight = 0; num_eight < 8; num_eight++) {
        if (num_eight < 4) {
            now_center.y = center.y;
            y_sign = 1;
        }  else {
            now_center.y = mirror_center.y;
            y_sign = -1;
        }
        if ((num_eight % 6) <= 1) {
            now_center.x = center.x;
            x_sign = 1;                      
        } else {
            now_center.x = mirror_center.x;
            x_sign = -1;                     
        }
        if ((num_eight % 4) % 3) {
            px = &cx; py = &cy;
        } else {
            px = &cy; py = &cx;
        }
        cy = start_po[num_eight].y;
        cx = start_po[num_eight].x;
        d = 4 * cx * cx + 4 * cy * cy - 4 * cy + 2 - diameter * diameter;
        dx = 8 * cx + 4;
        dy = -8 * cy + 8;
        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        r = NUM2INT(rb_ary_entry(color, 0)); 
        g = NUM2INT(rb_ary_entry(color, 1)); 
        b = NUM2INT(rb_ary_entry(color, 2));
        a = ( RARRAY_LENINT(color) == 4 ? NUM2INT(RARRAY_PTR(color)[3]) : 255);
        c_color = (a << 24) + (r << 16) + (g << 8) + b;
        buff = pixels;
        for (; cx <= end_po[num_eight].x; cx++) {
            if (d > 0) {
                d += dy; dy += 8;
                cy--;
            }
            buff[((*py) * y_sign + now_center.y ) * imageData->rect->w + ( (*px) * x_sign + now_center.x )] = c_color;
            d += dx;
            dx += 8;
        }
        SDL_UnlockTexture(texture);
    }
    return self;
}

static VALUE image_get_pixel(VALUE self, VALUE rb_x, VALUE rb_y) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    int x = NUM2INT(rb_x), y = NUM2INT(rb_y);
    if (x < 0 || x >= imageData->rect->w || y < 0 || y >= imageData->rect->h ) return Qnil;
    void *pixels; int pitch;
    SDL_LockTexture(imageData->texture, NULL, &pixels, &pitch);
    Uint32* buff = pixels;
    Uint8 r, g, b, a;
    SDL_PixelFormat * fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    SDL_GetRGBA(buff[imageData->rect->w*y+x] , fmt, &r, &g, &b, &a);
    SDL_FreeFormat(fmt);
    SDL_UnlockTexture(imageData->texture);
    if (a == 255) {
        return rb_ary_new3(3, rb_int_new(r), rb_int_new(g), rb_int_new(b));
    } else {
        return rb_ary_new3(4, rb_int_new(r), rb_int_new(g), rb_int_new(b), rb_int_new(a));
    }
}

static VALUE image_set_pixel(VALUE self, VALUE rb_x, VALUE rb_y, VALUE rb_color) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    int x = NUM2INT(rb_x), y = NUM2INT(rb_y);
    if (x < 0 || x >= imageData->rect->w || y < 0 || y >= imageData->rect->h ) return Qnil;
    void *pixels; int pitch;
    SDL_LockTexture(imageData->texture, NULL, &pixels, &pitch);
    Uint32* buff = pixels;
    Uint8 r = NUM2INT(rb_ary_entry(rb_color, 0)); 
    Uint8 g = NUM2INT(rb_ary_entry(rb_color, 1)); 
    Uint8 b= NUM2INT(rb_ary_entry(rb_color, 2));
    Uint8 a = ( RARRAY_LENINT(rb_color) == 4 ? NUM2INT(rb_ary_entry(rb_color, 3)) : 255);
    SDL_PixelFormat * fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    Uint32 color = SDL_MapRGBA(fmt, r, g, b, a);
    SDL_FreeFormat(fmt);
    buff[imageData->rect->w* y + x] = color;
    SDL_UnlockTexture(imageData->texture);

    return rb_color;
}

static VALUE image_draw_fill_circle(VALUE self, VALUE rb_x, VALUE rb_y, VALUE rb_radius, VALUE rb_color) {
    image_draw_circle(self, rb_x, rb_y, rb_radius, rb_color);
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    int x = NUM2INT(rb_x), y = NUM2INT(rb_y);
    void *pixels; int pitch;
    SDL_LockTexture(imageData->texture, NULL, &pixels, &pitch);
    Uint32 *buff = pixels;
    Uint8 r, g, b, a;
    r = NUM2INT(RARRAY_PTR(rb_color)[0]); 
    g = NUM2INT(RARRAY_PTR(rb_color)[1]); 
    b = NUM2INT(RARRAY_PTR(rb_color)[2]);
    a = ( RARRAY_LENINT(rb_color) == 4 ? NUM2INT(RARRAY_PTR(rb_color)[3]) : 255);
    Uint32 startColor, fillColor;
    startColor = buff[imageData->rect->w*y+x];
    fillColor = (a << 24) + (r << 16) + (g << 8) + b;

    FillShapeScanLine(buff, x, y, imageData->rect->w, imageData->rect->h, fillColor, startColor);
    SDL_UnlockTexture(imageData->texture);
    return self;
}

static VALUE image_draw_box(VALUE self, VALUE x, VALUE y, VALUE w, VALUE h, VALUE color) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    int c_x = NUM2INT(x);int c_y = NUM2INT(y);
    void *pixels; int pitch;
    SDL_LockTexture(imageData->texture, NULL, &pixels, &pitch);
    Uint8 r = RARRAY_PTR(color)[0]; Uint8 g = RARRAY_PTR(color)[1]; Uint8 b= RARRAY_PTR(color)[2];
    SDL_PixelFormat * fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    Uint32 c_color = SDL_MapRGBA(fmt, r, g, b, 255);
    SDL_FreeFormat(fmt);

    BOOL upDrawAble, rightDrawAble, downDrawAble, leftDrawAble;
    upDrawAble = rightDrawAble = downDrawAble = leftDrawAble = TRUE;

    int c_w = NUM2INT(w);int c_h = NUM2INT(h);
    if (c_w < 0) { c_x =+ c_w; c_w = abs(c_w) + c_x; }
    if (c_h < 0) { c_y =+ c_h; c_h = abs(c_h) + c_y; }
    if (c_x < 0) c_x = 0;
    if (c_y < 0) c_y = 0;
    if (c_x + c_w > imageData->rect->w) c_w -= (c_x + c_w) - imageData->rect->w;
    if (c_y + c_h > imageData->rect->h) c_h -= (c_y + c_h) - imageData->rect->h;

    Uint32 *buff = pixels; Uint32 line[c_w];
    int i; for (i = 0; i < c_w; ++i) line[i] = c_color;
    int sprite_w = imageData->rect->w;
    size_t diff = c_x + (sprite_w * c_y);
    size_t size = sizeof(uint32_t);

    if ( upDrawAble ) { memcpy(buff + diff, line, size * c_w); }
    if ( downDrawAble ) { memcpy(buff + diff + (c_h-1) * sprite_w, line, size * c_w); }
    for (i = 0; i < c_h; ++i) {
        if (leftDrawAble) { buff[diff + i * sprite_w] = c_color; }
        if (rightDrawAble) { buff[diff + (c_w-1) + i * sprite_w] = c_color; }
    }

    SDL_UnlockTexture(imageData->texture);
    return self;

}

static VALUE image_draw_fill(VALUE self, VALUE color) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    int w = imageData->rect->w, h = imageData->rect->h;

    void *pixels; int pitch;
    SDL_LockTexture(imageData->texture, NULL, &pixels, &pitch);
    Uint8 a, r, g, b;
    r = NUM2UINT(RARRAY_PTR(color)[0]); 
    g = NUM2UINT(RARRAY_PTR(color)[1]); 
    b = NUM2UINT(RARRAY_PTR(color)[2]);
    a = ( RARRAY_LENINT(color) == 4 ? NUM2INT(RARRAY_PTR(color)[3]) : 255);
    
    SDL_PixelFormat * fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    Uint32 c_color = SDL_MapRGBA(fmt, r, g, b, a);
    SDL_FreeFormat(fmt);
    Uint32 *buff = pixels; Uint32 dst[w];
     int i; for (i = 0; i < w; ++i) dst[i] = c_color;
   
    size_t size = sizeof(uint32_t); 
    for (i = 0; i < h; ++i) memcpy(buff + i * w, dst, size * w);
    SDL_UnlockTexture(imageData->texture);
    return self;
}
static VALUE image_draw_fill_box(VALUE self, VALUE x, VALUE y, VALUE w, VALUE h, VALUE color) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    int c_x = NUM2INT(x);int c_y = NUM2INT(y);
    void *pixels; int pitch;
    SDL_LockTexture(imageData->texture, NULL, &pixels, &pitch);
    Uint8 r = NUM2INT(rb_ary_entry(color, 0)); 
    Uint8 g = NUM2INT(rb_ary_entry(color, 1)); 
    Uint8 b= NUM2INT(rb_ary_entry(color, 2));
    Uint8 a = ( RARRAY_LENINT(color) == 4 ? NUM2INT(rb_ary_entry(color, 3)) : 255);

    SDL_PixelFormat * fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    Uint32 c_color = SDL_MapRGBA(fmt, r, g, b, a);
    SDL_FreeFormat(fmt);
    
    int c_w = NUM2INT(w);int c_h = NUM2INT(h);
    if (c_w < 0) { c_x =+ c_w; c_w = abs(c_w) + c_x; }
    if (c_h < 0) { c_y =+ c_h; c_h = abs(c_h) + c_y; }
    if (c_x < 0) c_x = 0;
    if (c_y < 0) c_y = 0;
    if (c_x + c_w > imageData->rect->w) c_w -= (c_x + c_w) - imageData->rect->w;
    if (c_y + c_h > imageData->rect->h) c_h -= (c_y + c_h) - imageData->rect->h;
    Uint32 *buff = pixels; Uint32 line[c_w];
    int i; for (i = 0; i < c_w; ++i) line[i] = c_color;
    int sprite_w = imageData->rect->w;
    size_t diff = c_x + (sprite_w * c_y);
    size_t size = sizeof(uint32_t); 

    for (i = 0; i < c_h; ++i) memcpy(buff + diff + i * sprite_w, line, size * c_w);
    SDL_UnlockTexture(imageData->texture);
    return self;
}

static VALUE image_draw_text(int argc, VALUE argv[], VALUE self) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    VALUE rb_message, rb_x, rb_y, rb_font, rb_color;
    if ( rb_scan_args(argc, argv, "41", &rb_message, &rb_x, &rb_y, &rb_font, &rb_color) == 4 ) {
        rb_color = rb_ary_new3(4, rb_int_new(255), rb_int_new(255), rb_int_new(255), rb_int_new(255));
    }
    if (RSTRING_LEN(rb_message) == 0) return Qnil;

    SDL_Color fontColor;
    fontColor.r = NUM2INT(rb_ary_entry(rb_color, 0));
    fontColor.g = NUM2INT(rb_ary_entry(rb_color, 1));
    fontColor.b = NUM2INT(rb_ary_entry(rb_color, 2));
    if (NUM2INT(rb_funcall(rb_color, rb_intern("size"), 0)) == 3) {
        rb_funcall(rb_color, rb_intern("push"), 1, rb_int_new(255));
    }
    fontColor.a = NUM2INT(rb_ary_entry(rb_color, 3));
    FontData *fontData; Data_Get_Struct(rb_font, FontData, fontData);
    TTF_Font *font = fontData->font;

    volatile VALUE text_ary = rb_str_split(rb_message,"\n");
    int text_ary_size = RARRAY_LENINT(text_ary);
    int sy = NUM2INT(rb_y);

    void *pixels; int pitch;
    SDL_LockTexture(imageData->texture, imageData->rect, &pixels, &pitch);
    Uint32 *buff = pixels, *srcBuff = NULL;
    Uint32 colorKey = 0;
    VALUE rb_height = rb_funcall(rb_font, rb_intern("size"), 0);
    int x = NUM2INT(rb_x), y = 0, cHeight = NUM2INT(rb_height);
    int i,j,s, diff_w = 0, diff_h = 0; Uint32 color;
    SDL_Surface *surf = NULL;
    for (s = 0; s < text_ary_size; ++s) {
        VALUE rb_text = RARRAY_PTR(text_ary)[s];
        if (RSTRING_LEN(rb_text) == 0) continue;

        const char* message = StringValuePtr(rb_text);
        surf = TTF_RenderUTF8_Blended(font, message, fontColor);
        srcBuff = surf->pixels;
        Uint32 colorKeys[] = { srcBuff[0], srcBuff[surf->w-1], srcBuff[(surf->h-1) * surf->w], srcBuff[surf->h * surf->w - 1] };
        Uint32 colorNums[4] = { 0 }, colorNumsMax = 0;
        for (i = 0; i < 3; ++i) { 
          for (j = i+1; j < 4; ++j) {
            if (colorKeys[i] == colorKeys[j]) { colorNums[i]++; }
          }
        }

        for (i = 0; i < 4; ++i) {
          if (colorNums[i] > colorNumsMax) { 
            colorNumsMax = colorNums[i]; 
            colorKey = colorKeys[i];
          }
        }
        y = sy + cHeight * s;
        diff_w = x + surf->w; diff_h = y + surf->h;
        if (diff_w > imageData->rect->w) diff_w -= (diff_w - imageData->rect->w);
        if (diff_h > imageData->rect->h) diff_h -= (diff_h - imageData->rect->h);
        if (x < 0) x = 0;if (y < 0) y = 0;
        
        for (i = y; i < diff_h; ++i) {
            for (j = x; j < diff_w; ++j) {
                color = srcBuff[(i - y) * surf->w + (j - x) ];
                if (color != colorKey) { buff[i * imageData->rect->w + j] = color; }
            }
        }
        SDL_FreeSurface(surf);
    }
    SDL_UnlockTexture(imageData->texture);

    return self;
}
static VALUE image_get_tone(VALUE self) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    Uint8 r, g, b;
    SDL_GetTextureColorMod(imageData->texture, &r, &g, &b);
    return rb_ary_new3(3, rb_int_new(r),rb_int_new(g),rb_int_new(b));
}

static VALUE image_set_tone(VALUE self, VALUE color) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    Uint8 r = NUM2INT(rb_ary_entry(color, 0)); 
    Uint8 g = NUM2INT(rb_ary_entry(color, 1)); 
    Uint8 b= NUM2INT(rb_ary_entry(color, 2));
    SDL_SetTextureColorMod(imageData->texture, r, g, b);
    return self;
}

static VALUE image_get_blend(VALUE self) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    SDL_BlendMode mode;
    SDL_GetTextureBlendMode(imageData->texture, &mode);
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
static VALUE image_set_blend(VALUE self, VALUE blend) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
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
    SDL_SetTextureBlendMode(imageData->texture, mode);
    return blend;
}

static VALUE image_get_alpha(VALUE self, VALUE alpha) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    Uint8 c_alpha;
    SDL_GetTextureAlphaMod(imageData->texture, &c_alpha);
    return rb_int_new(c_alpha);
}

static VALUE image_set_alpha(VALUE self, VALUE alpha) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    SDL_SetTextureAlphaMod(imageData->texture, NUM2INT(alpha));
    return self;
}

static VALUE image_get_width(VALUE self) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    return rb_int_new(imageData->rect->w);
}

static VALUE image_get_height(VALUE self) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    return rb_int_new(imageData->rect->h);
}

static VALUE image_draw_roundbox(VALUE self, VALUE x, VALUE y, 
    VALUE width, VALUE height, VALUE radius, VALUE color)
{
    volatile VALUE r = radius, c = color;
    volatile VALUE x2 = rb_int_new( NUM2INT(x) + NUM2INT(width));
    volatile VALUE y2 = rb_int_new( NUM2INT(y) + NUM2INT(height));
    VALUE double_r = rb_int_new(NUM2INT(r)*2);
    VALUE zero = rb_int_new(0);
    volatile VALUE image = rb_class_new_instance(2, 
        (VALUE[]){ double_r, double_r  }, rb_path2class("SDLUdon::Image"));
    rb_funcall(image, rb_intern("circle"), 4, r, r, r, c);
    rb_funcall(self, rb_intern("draw"), 7, image, x, y, zero, zero, r, r);
    rb_funcall(self, rb_intern("draw"), 7, image, rb_funcall(x2, rb_intern("-"), 1, r), y, r, zero, r, r);
    rb_funcall(self, rb_intern("draw"), 7, image, rb_funcall(x2, rb_intern("-"), 1, r), rb_funcall(y2, rb_intern("-"), 1, r), r, r, r, r);
    rb_funcall(self, rb_intern("draw"), 7, image, x, rb_funcall(y2, rb_intern("-"), 1, r), zero, r, r, r);

    x2 = rb_int_new(NUM2INT(x2)-1);
    y2 = rb_int_new(NUM2INT(y2)-1);
    rb_funcall(self, rb_intern("line"), 5, rb_funcall(x, rb_intern("+"), 1, r), y, rb_funcall(x2, rb_intern("-"), 1, r), y, c);
    rb_funcall(self, rb_intern("line"), 5, x2, rb_funcall(y, rb_intern("+"), 1, r), x2, rb_funcall(y2, rb_intern("-"), 1, r), c);
    rb_funcall(self, rb_intern("line"), 5, rb_funcall(x2, rb_intern("-"), 1, r), y2, rb_funcall(x, rb_intern("+"), 1, r), y2, c);
    rb_funcall(self, rb_intern("line"), 5, x, rb_funcall(y, rb_intern("+"), 1, r), x, rb_funcall(y2, rb_intern("-"), 1, r), c);
    rb_funcall(image, rb_intern("dispose"), 0);
    return self;
}

static VALUE image_draw_fill_roundbox(VALUE self, VALUE x, VALUE y, 
    VALUE width, VALUE height, VALUE radius, VALUE color)
{
    volatile VALUE r = radius, c = color;
    volatile VALUE x2 = rb_int_new( NUM2INT(x) + NUM2INT(width));
    volatile VALUE y2 = rb_int_new( NUM2INT(y) + NUM2INT(height));
    volatile VALUE double_r = rb_int_new(NUM2INT(r)*2);
    volatile VALUE zero = rb_int_new(0);
    volatile VALUE image = rb_class_new_instance(2, 
        (VALUE[]){ double_r, double_r  }, rb_path2class("SDLUdon::Image"));    
    rb_funcall(image, rb_intern("fill_circle"), 4, r, r, r, c);
    rb_funcall(self, rb_intern("draw"), 7, image, x, y, zero, zero, r, r);
    rb_funcall(self, rb_intern("draw"), 7, image, rb_funcall(x2, rb_intern("-"), 1, r), y, r, zero, r, r);
    rb_funcall(self, rb_intern("draw"), 7, image, rb_funcall(x2, rb_intern("-"), 1, r), rb_funcall(y2, rb_intern("-"), 1, r), r, r, r, r);
    rb_funcall(self, rb_intern("draw"), 7, image, x, rb_funcall(y2, rb_intern("-"), 1, r), zero, r, r, r);

    rb_funcall(self, rb_intern("fill_box"), 5, rb_funcall(x, rb_intern("+"), 1, r), y, rb_funcall(width, rb_intern("-"), 1, double_r), height, c);
    rb_funcall(self, rb_intern("fill_box"), 5, x, rb_funcall(y, rb_intern("+"), 1, r), width, rb_funcall(height, rb_intern("-"), 1, double_r), c);
    rb_funcall(image, rb_intern("dispose"), 0);
    return self;
}

static VALUE image_dup(VALUE image) {
    volatile VALUE width = rb_funcall(image, rb_intern("width"), 0);
    volatile VALUE height = rb_funcall(image, rb_intern("height"), 0);
    volatile VALUE dup_image = rb_class_new_instance(2, (VALUE []){ width, height }, rb_class_of(image));
    rb_funcall(dup_image, rb_intern("copy"), 3, image, rb_int_new(0), rb_int_new(0));
    return dup_image;
}

static VALUE image_clear(VALUE self) {
    ImageData* imageData; Data_Get_Struct(self, ImageData, imageData);
    void *pixels; int pitch, i;
    SDL_LockTexture(imageData->texture, imageData->rect, &pixels, &pitch);
    Uint32 *buff = pixels;
    SDL_PixelFormat * fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    Uint32 color = SDL_MapRGBA(fmt, 0, 0, 0, 0);
    SDL_FreeFormat(fmt);
    for (i = 0; i < imageData->rect->w * imageData->rect->h; i++) buff[i] = color;
    SDL_UnlockTexture(imageData->texture);
    return Qnil;
}

void Init_image(VALUE parent_module) {
    VALUE image = rb_define_class_under(parent_module, "Image", rb_cObject);
    rb_define_alloc_func(image, image_alloc);
    rb_define_private_method(image, "initialize", image_initialize, -1);
    rb_define_singleton_method(image, "load", image_load, -1);
    rb_define_singleton_method(image, "text", image_class_text, -1);

    rb_define_method(image, "dispose", image_dispose, 0);
    rb_define_method(image, "save", image_save, 1);
    rb_define_method(image, "scan", image_scan, 4);
    rb_define_method(image, "scan_tiles", image_scan_tiles, -1);
    rb_define_method(image, "draw", image_draw, -1);
    rb_define_method(image, "copy", image_draw_copy, -1);
    rb_define_method(image, "point", image_draw_point, 3);
    rb_define_method(image, "line", image_draw_line, 5);
    rb_define_method(image, "circle", image_draw_circle, 4);
    rb_define_method(image, "fill_circle", image_draw_fill_circle, 4);
    rb_define_method(image, "box", image_draw_box, 5);
    rb_define_method(image, "fill", image_draw_fill, 1); 
    rb_define_method(image, "fill_box", image_draw_fill_box, 5);
    rb_define_method(image, "text", image_draw_text, -1);
    rb_define_method(image, "[]", image_get_pixel, 2);
    rb_define_method(image, "[]=", image_set_pixel, 3);
    rb_define_method(image, "alpha", image_get_alpha, 0);
    rb_define_method(image, "alpha=", image_set_alpha, 1);
    rb_define_method(image, "width", image_get_width, 0);
    rb_define_method(image, "height", image_get_height, 0);
    rb_define_method(image, "roundbox", image_draw_roundbox, 6);
    rb_define_method(image, "fill_roundbox", image_draw_fill_roundbox, 6);
    rb_define_method(image, "tone=", image_set_tone, 1);
    rb_define_method(image, "tone", image_get_tone, 0);
    rb_define_method(image, "blend=", image_set_blend, 1);
    rb_define_method(image, "blend", image_get_blend, 0);
    rb_define_method(image, "dup", image_dup, 0);
    rb_define_method(image, "clone", image_dup, 0);
    rb_define_method(image, "clear", image_clear, 0);
}
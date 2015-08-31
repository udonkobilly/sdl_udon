#include "sdl_udon_private.h"

static const char* WIN_FONTPATH = "C:/Windows/Fonts/";
static st_table* winfontNameMap = NULL;
static FontCache *fontCache = NULL;

void FreeFontCache() {
    FontCache *buff = fontCache;
    while (buff != NULL) {
        FontCache *next = buff->next;
        TTF_CloseFont(buff->font);
        buff->next = NULL;
        free(buff);
        buff = next;
    }
}

static VALUE font_alloc(VALUE self) {
    FontData *fontData = ALLOC(FontData);
    fontData->font = NULL;
    fontData->name = NULL;
    return Data_Wrap_Struct(self, 0, -1, fontData);
}

static FontCache* createFontSetCache(const char *name, const int size, const int index) {
    FontCache *cache = malloc(sizeof(FontCache));
    cache->font = TTF_OpenFontIndex(name, size, index);
    if (cache->font == NULL) SDL_LOG_ABORT();
    cache->size = size;
    size_t len = strlen(name);
    cache->name = malloc(sizeof(char)*len);
    strncpy(cache->name, name, len);
    cache->next = NULL;
    return cache;
}

static VALUE font_initialize(int argc, VALUE argv[], VALUE self) {
    FontData *fontData; Data_Get_Struct(self, FontData, fontData);
    VALUE name;
    volatile VALUE size, opt;
    rb_scan_args(argc, argv, "12", &name, &size, &opt);
    if (NIL_P(opt)) opt = rb_hash_new();
    int cSize = NIL_P(size) ? 12 : NUM2INT(size); 
    volatile VALUE rIndex = rb_hash_lookup(opt, ID2SYM(rb_intern("index")));
    int index = NIL_P(rIndex) ? -1 : NUM2INT(rIndex);
    char *cName = NULL;
    VALUE flag = rb_funcall(name, rb_intern("=~"), 1, rb_reg_new("\\.tt[fc]$", strlen("\\.tt[fc]$"), 0));
    if (NIL_P(flag)) {
        const char *nameKey = StringValuePtr(name);
        st_data_t data;
        st_lookup(winfontNameMap, (st_data_t)nameKey,&data);
        if (NIL_P(data)) { rb_raise(rb_eException, "fontname not found..."); }
        volatile VALUE strs = rb_str_split(rb_str_new2((const char*)data), "|");
        if (index == -1) index = NUM2INT(rb_str2inum(RARRAY_PTR(strs)[1], 10));
        volatile VALUE temp = rb_str_concat(rb_str_new2(WIN_FONTPATH), RARRAY_PTR(strs)[0]);
        cName = StringValuePtr(temp);
        size_t nameKeyLen = strlen(nameKey);
        fontData->name = malloc(nameKeyLen);
        strncpy(fontData->name, nameKey, nameKeyLen);
    } else {
        index = 0;
        cName = StringValuePtr(name);
    }

    if (fontCache == NULL) {
        fontCache = createFontSetCache(cName, cSize, index);
        fontData->font = fontCache->font;
        rb_funcall(self, rb_intern("style="), 1, opt);
    } else {
        FontCache *buff = fontCache;
        while (TRUE) {
            size_t nameSize = strlen(buff->name);
            if ((strncmp(buff->name, cName,nameSize) == 0) && (buff->size == cSize) ) {
                fontData->font = buff->font;
                break;
            }
            if (buff->next == NULL) {
                buff->next = createFontSetCache(cName, cSize, index);
                fontData->font = buff->next->font;
                rb_funcall(self, rb_intern("style="), 1, opt);
                break;
            }
            buff = buff->next;    
        }
    }
    return Qnil;
}

static VALUE font_size(VALUE self) {
    FontData *fontData; Data_Get_Struct(self, FontData, fontData);
    return rb_int_new(TTF_FontHeight(fontData->font));
}

static VALUE font_get_style(VALUE self) {
    FontData *fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    volatile VALUE result = rb_hash_new();
    rb_hash_aset(result, ID2SYM(rb_intern("bold")), (style & TTF_STYLE_BOLD) ? Qtrue : Qfalse);
    rb_hash_aset(result, ID2SYM(rb_intern("italic")), (style & TTF_STYLE_ITALIC) ? Qtrue : Qfalse);
    rb_hash_aset(result, ID2SYM(rb_intern("underline")), (style & TTF_STYLE_UNDERLINE) ? Qtrue : Qfalse);
    rb_hash_aset(result, ID2SYM(rb_intern("strikethrough")), (style & TTF_STYLE_STRIKETHROUGH) ? Qtrue : Qfalse);
    return result;
}

static VALUE font_set_style(VALUE self, VALUE hash) {
    FontData *fontData; Data_Get_Struct(self, FontData, fontData);
    int style = 0;
    VALUE bool_bold = rb_hash_lookup(hash, ID2SYM(rb_intern("bold")));
    if (bool_bold == Qtrue) style = style | 0x01;
    VALUE bool_italic = rb_hash_lookup(hash, ID2SYM(rb_intern("italic")));
    if (bool_italic == Qtrue) style = style | 0x02; 
    VALUE bool_underline = rb_hash_lookup(hash, ID2SYM(rb_intern("underline")));
    if (bool_underline == Qtrue) style = style | 0x04; 
    VALUE bool_strikethrough = rb_hash_lookup(hash, ID2SYM(rb_intern("strikethrough")));
    if (bool_strikethrough == Qtrue) style = style | 0x08; 
    TTF_SetFontStyle(fontData->font, style);
    return self;
}

static VALUE font_get_outline(VALUE self, VALUE hash) {
    FontData *fontData; Data_Get_Struct(self, FontData, fontData);
    return rb_int_new(TTF_GetFontOutline(fontData->font) );
}

static VALUE font_set_outline(VALUE self, VALUE num) {
    FontData *fontData; Data_Get_Struct(self, FontData, fontData);
    TTF_SetFontOutline(fontData->font, NUM2INT(num));
    return self;
}

static VALUE font_get_hinting(VALUE self) {
    FontData *fontData; Data_Get_Struct(self, FontData, fontData);
    return rb_int_new(TTF_GetFontHinting(fontData->font));
}

static VALUE font_set_hinting(VALUE self, VALUE num_or_sym) {
    int result;
    if (rb_type(num_or_sym) == T_SYMBOL) {
        volatile VALUE str = rb_sym_to_s(num_or_sym);
        const char* sym_str = StringValuePtr( str );
        size_t len = strlen(sym_str);
        if (strncmp(sym_str, "normal", len) == 0) {
            result = 0;
        } else if (strncmp(sym_str, "light", len) == 0) {
            result = 1;
        } else if (strncmp(sym_str, "mono", len) == 0) {
            result = 2;
        } else if (strncmp(sym_str, "none", len) == 0) {
            result = 3;
        } else {
            result = 0;
        }
    } else {
        result = NUM2INT(num_or_sym);
    }
    FontData *fontData; Data_Get_Struct(self, FontData, fontData);
    TTF_SetFontHinting(fontData->font, result);
    return self;
}

static VALUE font_get_ascent(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    return rb_int_new(TTF_FontAscent(fontData->font));
}

static VALUE font_get_descent(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    return rb_int_new(TTF_FontDescent(fontData->font));
}

static VALUE font_get_line_skip(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    return rb_int_new(TTF_FontLineSkip(fontData->font));
}

static VALUE font_get_kerning(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    return rb_int_new(TTF_GetFontKerning(fontData->font));
}

static VALUE font_set_kerning(VALUE self, VALUE num) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    TTF_SetFontKerning(fontData->font, NUM2INT(num) );
    return self;
}

static VALUE font_get_number(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    return rb_int_new(TTF_FontFaces(fontData->font));
}

static VALUE font_is_fixed_width(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    return (TTF_FontFaceIsFixedWidth(fontData->font) ? Qtrue : Qfalse);
}
static VALUE font_get_name(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    if (fontData->name == NULL) {
        return rb_str_new2(TTF_FontFaceFamilyName( fontData->font ));
    } else {
        VALUE volatile str = rb_str_new2( fontData->name);
        rb_funcall(str, rb_intern("force_encoding"), 1, rb_str_new2("utf-8"));
        return str;
    }
}

static VALUE font_get_default_style(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    VALUE str = rb_str_new_cstr(TTF_FontFaceStyleName(fontData->font ));
    return rb_funcall( rb_funcall(str, rb_intern("downcase"), 0), rb_intern("to_sym"), 0);
}

static VALUE font_clear_style(VALUE self) {
    volatile VALUE hash = rb_hash_new();
    rb_hash_aset(hash, rb_funcall(self, rb_intern("default_style"), 0), Qtrue);
    rb_funcall(self, rb_intern("style="), 1, hash);
    return Qnil;
}

static VALUE font_is_regular(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    return (style == 0 ? Qtrue : Qfalse);    
}

static VALUE font_set_regular(VALUE self, VALUE boolean) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    if (boolean == Qtrue) { style = 0; }
    TTF_SetFontStyle(fontData->font, style);
    return Qnil;
}

static VALUE font_is_bold(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    return (style == 0x01 ? Qtrue : Qfalse);    
}

static VALUE font_set_bold(VALUE self, VALUE boolean) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    style = ( boolean == Qtrue ? (style | 0x01) : (style & ~0x01) );
    TTF_SetFontStyle(fontData->font, style);
    return Qnil;
}

static VALUE font_is_italic(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    return (style == 0x02 ? Qtrue : Qfalse);    
}

static VALUE font_set_italic(VALUE self, VALUE boolean) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    style = (boolean == Qtrue ? (style | 0x02) : (style & ~0x02) );
    TTF_SetFontStyle(fontData->font, style);
    return Qnil;
}

static VALUE font_is_underline(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    return (style == 0x02 ? Qtrue : Qfalse);    
}

static VALUE font_set_underline(VALUE self, VALUE boolean) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    style = ( boolean == Qtrue ? (style | 0x04) : (style & ~0x04) );
    TTF_SetFontStyle(fontData->font, style);
    return Qnil;
}

static VALUE font_is_strikethrough(VALUE self) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    return (style == 0x08 ? Qtrue : Qfalse);    
}

static VALUE font_set_strikethrough(VALUE self, VALUE boolean) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int style = TTF_GetFontStyle(fontData->font);
    style = (boolean == Qtrue ? style | 0x08 : style & ~0x08 );
    TTF_SetFontStyle(fontData->font, style);
    return Qnil;
}

static VALUE font_get_text_width(VALUE self, VALUE text) {
    FontData* fontData; Data_Get_Struct(self, FontData, fontData);
    int w, h;
    TTF_SizeUTF8(fontData->font, StringValuePtr(text), &w, &h);
    return rb_int_new(w);
}

void Quit_font() { st_free_table(winfontNameMap); }

void Init_font(VALUE parent) {
    VALUE font_class = rb_define_class_under(parent, "Font", rb_cObject);
//    rb_define_singleton_method(font_class, "exist?", font_class_is_exist, 1);
    rb_define_alloc_func(font_class, font_alloc);
    rb_define_private_method(font_class, "initialize", font_initialize, -1);
    rb_define_method(font_class, "size", font_size, 0);
    rb_define_method(font_class, "style", font_get_style, 0);
    rb_define_method(font_class, "style=", font_set_style, 1);
    rb_define_method(font_class, "outline", font_get_outline, 0);
    rb_define_method(font_class, "outline=", font_set_outline, 1);
    rb_define_method(font_class, "hinting", font_get_hinting, 0);
    rb_define_method(font_class, "hinting=", font_set_hinting, 1);
    rb_define_method(font_class, "ascent", font_get_ascent, 0);
    rb_define_method(font_class, "descent", font_get_descent, 0);
    rb_define_method(font_class, "line_skip", font_get_line_skip, 0);
    rb_define_method(font_class, "kerning", font_get_kerning, 0);
    rb_define_method(font_class, "kerning=", font_set_kerning, 1);
    rb_define_method(font_class, "number", font_get_number, 0);
    rb_define_method(font_class, "fixed_width?", font_is_fixed_width, 0);
    rb_define_method(font_class, "name", font_get_name, 0);
    rb_define_method(font_class, "default_style", font_get_default_style, 0);
    rb_define_method(font_class, "clear_style", font_clear_style, 0);
    rb_define_method(font_class, "regular?", font_is_regular, 0);
    rb_define_method(font_class, "regular=", font_set_regular, 0);
    rb_define_method(font_class, "bold?", font_is_bold, 0);
    rb_define_method(font_class, "bold=", font_set_bold, 1);
    rb_define_method(font_class, "italic?", font_is_italic, 0);
    rb_define_method(font_class, "italic=", font_set_italic, 1);
    rb_define_method(font_class, "underline?", font_is_underline, 0);
    rb_define_method(font_class, "underline=", font_set_underline, 1);
    rb_define_method(font_class, "strikethrougu?", font_is_strikethrough, 0);
    rb_define_method(font_class, "strikethrougu=", font_set_strikethrough, 1);
    rb_define_method(font_class, "text_width", font_get_text_width, 1);

    winfontNameMap = st_init_strtable_with_size(106);
    st_add_direct(winfontNameMap,(st_data_t)"CRバジョカ隅書体",(st_data_t)"CRBajoka-G.ttf|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRバジョカ廉書体",(st_data_t)"CRBajoka-R.ttf|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇブーケ",(st_data_t)"CRccgbqb.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRPＣ＆Ｇブーケ",(st_data_t)"CRccgbqb.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇブーケ04",(st_data_t)"CRCCGBQB4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRPＣ＆Ｇブーケ04",(st_data_t)"CRCCGBQB4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇれいしっく",(st_data_t)"CRccgrtr.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRPＣ＆Ｇれいしっく",(st_data_t)"CRccgrtr.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇれいしっく04",(st_data_t)"CRCCGRTR4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRPＣ＆Ｇれいしっく04",(st_data_t)"CRCCGRTR4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇ流麗太行書体",(st_data_t)"CRccgryb.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRPＣ＆Ｇ流麗太行書体",(st_data_t)"CRccgryb.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇ流麗太行書体04",(st_data_t)"CRCCGRYB4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRPＣ＆Ｇ流麗太行書体04",(st_data_t)"CRCCGRYB4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇ流麗行書体",(st_data_t)"CRccgrym.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRPＣ＆Ｇ流麗行書体",(st_data_t)"CRccgrym.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇ流麗行書体04",(st_data_t)"CRCCGRYM4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRPＣ＆Ｇ流麗行書体04",(st_data_t)"CRCCGRYM4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇ行刻",(st_data_t)"CRCCGyok.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRPＣ＆Ｇ行刻",(st_data_t)"CRCCGyok.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇ由紀葉太楷書体04",(st_data_t)"CRCCGYUK4.ttf|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRＣ＆Ｇ行刻04",(st_data_t)"CRCGYOK4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"CRPＣ＆Ｇ行刻04",(st_data_t)"CRCGYOK4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦクラフト墨W9",(st_data_t)"DF-CraftSumi-W9.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＰクラフト墨W9",(st_data_t)"DF-CraftSumi-W9.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＧクラフト墨W9",(st_data_t)"DF-CraftSumi-W9.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦ太丸ゴシック体",(st_data_t)"DF-FutoMaruGothic-W9.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＰ太丸ゴシック体",(st_data_t)"DF-FutoMaruGothic-W9.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＧ太丸ゴシック体",(st_data_t)"DF-FutoMaruGothic-W9.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦ行楷書W5",(st_data_t)"DF-GyouKaiSho-W5.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＰ行楷書W5",(st_data_t)"DF-GyouKaiSho-W5.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＧ行楷書W5",(st_data_t)"DF-GyouKaiSho-W5.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦ行書体W7",(st_data_t)"DF-GyouSho-W7.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＰ行書体W7",(st_data_t)"DF-GyouSho-W7.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＧ行書体W7",(st_data_t)"DF-GyouSho-W7.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦ華康ゴシック体W5",(st_data_t)"DF-KaKouGothic-W5.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＰ華康ゴシック体W5",(st_data_t)"DF-KaKouGothic-W5.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＧ華康ゴシック体W5",(st_data_t)"DF-KaKouGothic-W5.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦ金文体W5",(st_data_t)"DF-KinBun-W5.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＰ金文体W5",(st_data_t)"DF-KinBun-W5.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＧ金文体W5",(st_data_t)"DF-KinBun-W5.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦPOPミックスW4",(st_data_t)"DF-PopMix-W4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＰPOPミックスW4",(st_data_t)"DF-PopMix-W4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＧPOPミックスW4",(st_data_t)"DF-PopMix-W4.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦロマン雪W9",(st_data_t)"DF-RomanYuki-W9.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＰロマン雪W9",(st_data_t)"DF-RomanYuki-W9.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＧロマン雪W9",(st_data_t)"DF-RomanYuki-W9.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦ流隷体W5",(st_data_t)"DF-RuRei-W5.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＰ流隷体W5",(st_data_t)"DF-RuRei-W5.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＤＦＧ流隷体W5",(st_data_t)"DF-RuRei-W5.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"AR丸ゴシック体M04",(st_data_t)"jgtr00m4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP丸ゴシック体M04",(st_data_t)"jgtr00m4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR悠々ゴシック体E",(st_data_t)"jhop00e.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP悠々ゴシック体E",(st_data_t)"jhop00e.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR悠々ｺﾞｼｯｸ体E04",(st_data_t)"jhop00e4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP悠々ｺﾞｼｯｸ体E04",(st_data_t)"jhop00e4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR楷書体M",(st_data_t)"jkai00m.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP楷書体M",(st_data_t)"jkai00m.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR楷書体M04",(st_data_t)"jkai00m4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP楷書体M04",(st_data_t)"jkai00m4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR勘亭流H04",(st_data_t)"jkan00h4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP勘亭流H04",(st_data_t)"jkan00h4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR隷書体M",(st_data_t)"jlei00m.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP隷書体M",(st_data_t)"jlei00m.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR隷書体M04",(st_data_t)"jlei00m4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP隷書体M04",(st_data_t)"jlei00m4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ARハイカラＰＯＰ体H",(st_data_t)"jpom00h.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARPハイカラＰＯＰ体H",(st_data_t)"jpom00h.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ARハイカラPOP体H04",(st_data_t)"jpom00h4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARPハイカラPOP体H04",(st_data_t)"jpom00h4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ARＰＯＰ４B",(st_data_t)"jpop04b.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARPＰＯＰ４B",(st_data_t)"jpop04b.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ARＰＯＰ４B04",(st_data_t)"jpop04b4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARPＰＯＰ４B04",(st_data_t)"jpop04b4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR行書体B",(st_data_t)"jsin00b.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP行書体B",(st_data_t)"jsin00b.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR行書体B04",(st_data_t)"jsin00b4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP行書体B04",(st_data_t)"jsin00b4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR行楷書体H04",(st_data_t)"jska00h4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP行楷書体H04",(st_data_t)"jska00h4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR宋朝体M",(st_data_t)"jsun00m.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP宋朝体M",(st_data_t)"jsun00m.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR宋朝体M04",(st_data_t)"jsun00m4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP宋朝体M04",(st_data_t)"jsun00m4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"AR丸印篆B04",(st_data_t)"JWYZ00B4.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ARP丸印篆B04",(st_data_t)"JWYZ00B4.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"メイリオ",(st_data_t)"meiryo.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"メイリオイタリック",(st_data_t)"meiryo.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"MeiryoUI",(st_data_t)"meiryo.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"MeiryoUIItalic",(st_data_t)"meiryo.ttc|3");
    st_add_direct(winfontNameMap,(st_data_t)"メイリオボールド",(st_data_t)"meiryob.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"メイリオボールドイタリック",(st_data_t)"meiryob.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"MeiryoUIBold",(st_data_t)"meiryob.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"MeiryoUIBoldItalic",(st_data_t)"meiryob.ttc|3");
    st_add_direct(winfontNameMap,(st_data_t)"ＭＳゴシック",(st_data_t)"msgothic.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"MSUIGothic",(st_data_t)"msgothic.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"ＭＳＰゴシック",(st_data_t)"msgothic.ttc|2");
    st_add_direct(winfontNameMap,(st_data_t)"ＭＳ明朝",(st_data_t)"msmincho.ttc|0");
    st_add_direct(winfontNameMap,(st_data_t)"ＭＳＰ明朝",(st_data_t)"msmincho.ttc|1");
    st_add_direct(winfontNameMap,(st_data_t)"OCR-B",(st_data_t)"Ocr-b.ttf|0");
    st_add_direct(winfontNameMap,(st_data_t)"游ゴシックBold",(st_data_t)"yugothib.ttf|0");
    st_add_direct(winfontNameMap,(st_data_t)"游ゴシックRegular",(st_data_t)"yugothic.ttf|0");
    st_add_direct(winfontNameMap,(st_data_t)"游ゴシックLight",(st_data_t)"yugothil.ttf|0");
    st_add_direct(winfontNameMap,(st_data_t)"游明朝Regular",(st_data_t)"yumin.ttf|0");
    st_add_direct(winfontNameMap,(st_data_t)"游明朝Demibold",(st_data_t)"yumindb.ttf|0");
    st_add_direct(winfontNameMap,(st_data_t)"游明朝Light",(st_data_t)"yuminl.ttf|0");
}
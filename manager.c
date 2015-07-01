#include "sdl_udon_private.h"

static VALUE module_manager_init_manage(VALUE self) {
    volatile VALUE loader = rb_class_new_instance(0, NULL, rb_path2class("SDLUdon::Manager::Loader"));
    rb_ivar_set(self, rb_intern("@_loader"), loader);
    return Qnil;
}

static VALUE module_manager_store(int argc, VALUE argv[], VALUE self) {
    volatile VALUE loader = rb_ivar_get(self, rb_intern("@_loader"));
    rb_funcall2(loader, rb_intern("set"), argc, argv);
    return Qnil;
}

static VALUE module_manager_fetch(int argc, VALUE argv[], VALUE self) {
    volatile VALUE loader = rb_ivar_get(self, rb_intern("@_loader"));
    volatile VALUE result = rb_funcall2(loader, rb_intern("get"), argc, argv);
    return result;
}

static VALUE module_manager_is_loaded(VALUE self) {
    volatile VALUE loader = rb_ivar_get(self, rb_intern("@_loader"));
    return rb_funcall(loader, rb_intern("complete?"), 0);
}

static VALUE module_manager_is_loading(VALUE self) {
    volatile VALUE loader = rb_ivar_get(self, rb_intern("@_loader"));
    return rb_funcall(loader, rb_intern("complete?"), 0) == Qtrue ? Qfalse : Qtrue;
}


static int FreeLoaderImageTable(st_data_t k, st_data_t v, st_data_t arg) {
    char* keyStr = (char*)k;
    free(keyStr);
    SDL_Surface* surface = (SDL_Surface*)v;
    SDL_FreeSurface(surface);
    return ST_CONTINUE;
}

static int FreeLoaderSETable(st_data_t k, st_data_t v, st_data_t arg) {
    char* keyStr = (char*)k;
    free(keyStr);
    Uint8 *buf = (Uint8*)v;
    free(buf);
    return ST_CONTINUE;
}

static void loader_dfree(LoaderData *loader)  {
    st_data_t data;
    st_lookup(loader->table, (st_data_t)"image", &data);
    st_table* imageTable = (st_table*)data;
    st_foreach(imageTable, FreeLoaderImageTable, (st_data_t)NULL);
    st_free_table(imageTable);
    st_lookup(loader->table, (st_data_t)"se", &data);
    st_table* seTable = (st_table*)data;
    st_foreach(seTable, FreeLoaderSETable, (st_data_t)NULL);
    st_free_table(seTable);
    st_lookup(loader->table, (st_data_t)"bgm", &data);
    st_table* bgmTable = (st_table*)data;
    st_free_table(bgmTable);

    st_free_table(loader->table);
}

static VALUE loader_alloc(VALUE klass) {
    LoaderData *loader = ALLOC(LoaderData);
    return Data_Wrap_Struct(klass, 0, loader_dfree, loader);
}

static VALUE load_proc_call(VALUE data) {
    return rb_funcall( data, rb_intern("call"), 0);
}

static VALUE rescue_load_proc_call(VALUE data) {
    return Qnil;
}



static VALUE loader_thread_block( VALUE val, VALUE loader, int argc, VALUE argv[]) {
    SleepEx(0, FALSE);
    volatile VALUE mutex = argv[0];
    volatile VALUE queue = argv[1];
    LoaderData *loaderData; Data_Get_Struct(loader, LoaderData, loaderData);
    st_table* table = loaderData->table;
    while (TRUE) {
        volatile VALUE ary = rb_funcall(queue, rb_intern("pop"), 0);
        rb_mutex_lock(mutex);
        const char* key = StringValuePtr( RARRAY_PTR(ary)[1] );
        const char* type = StringValuePtr( RARRAY_PTR(ary)[2] );
        volatile VALUE value = rb_rescue(load_proc_call, RARRAY_PTR(ary)[0], rescue_load_proc_call, Qnil);
        if (NIL_P(value)) break;
        st_data_t data;
        st_lookup(table, (st_data_t)type, &data);
        int bytesize = NUM2INT(rb_funcall(RARRAY_PTR(ary)[1], rb_intern("bytesize"), 0));
        char *insertKey = malloc(sizeof(char) * bytesize + 1);
        sprintf(insertKey, "%s", key);
        st_add_direct((st_table*)data, (st_data_t)insertKey, (st_data_t)value);
        if (rb_funcall(queue, rb_intern("empty?"), 0) == Qtrue) {
            loaderData->complete = TRUE;
            rb_thread_sleep(0);
        }
        rb_mutex_unlock(mutex);
    }
    if (!loaderData->complete) {
        loaderData->error = TRUE;
    }
    return Qnil;
}

static VALUE loader_initialize(VALUE self) {
    LoaderData *loader; Data_Get_Struct(self, LoaderData, loader);
    loader->table = st_init_strtable();
    loader->complete = loader->error = FALSE;

    st_insert(loader->table, (st_data_t)"image", (st_data_t)st_init_strtable());
    st_insert(loader->table, (st_data_t)"se", (st_data_t)st_init_strtable());
    st_insert(loader->table, (st_data_t)"bgm", (st_data_t)st_init_strtable());
    volatile VALUE mutex = rb_cvar_get(rb_class_of(self), rb_intern("@@_mutex"));
    volatile VALUE queue = rb_cvar_get(rb_class_of(self), rb_intern("@@_queue")); 
    volatile VALUE thread = rb_block_call(rb_cThread, rb_intern("start"), 2, (VALUE[]){ mutex, queue }, loader_thread_block, self);
    rb_ivar_set(self, rb_intern("@_thread"), thread);
    return Qnil;
}


static VALUE load_image(VALUE dummy, VALUE filename, int argc, VALUE argv[]) {
    SDL_Surface* surface = IMG_Load(StringValuePtr(filename));
    return (st_data_t)surface;
}

static VALUE load_music(VALUE dummy, VALUE filename, int argc, VALUE argv[]) {
    SDL_RWops *rw = SDL_RWFromFile(StringValuePtr(filename),"r");
    if (rw != NULL) {
        Sint64 size = rw->size(rw);
        size_t len = sizeof(Uint8) * size;
        Uint8 *buf = malloc(len + sizeof(Uint8)*4);
        SDL_RWread(rw, buf+4, len, 1);
        SDL_RWclose(rw);
        buf[0] = ((Uint32)len >> 24) & 0xff;
        buf[1] = ((Uint32)len >> 16) & 0xff;
        buf[2] = ((Uint32)len >> 8) & 0xff;
        buf[3] = (Uint32)len & 0xff;
        return (st_data_t)buf;
    } else {
        return Qnil;
    }
}

static VALUE load_effect(VALUE dummy, VALUE filename, int argc, VALUE argv[]) {
    SDL_RWops *rw = SDL_RWFromFile(StringValuePtr(filename),"r");
    if (rw != NULL) {
        Sint64 size = rw->size(rw);
        size_t len = sizeof(Uint8) * size;
        Uint8 *buf = malloc(len);
        SDL_RWread(rw, buf, len, 1);
        SDL_RWclose(rw);
        return (st_data_t)buf;
    } else {
        return Qnil;
    }
}

static void PushLoadTask(VALUE loader, VALUE access_key, VALUE filename, VALUE type) {
    if (rb_funcall(rb_cFile, rb_intern("exist?"), 1, filename) == Qfalse) return;
    VALUE extname = rb_funcall(rb_cFile, rb_intern("extname"), 1, filename);
    if (NIL_P(access_key))
        access_key = rb_funcall(rb_cFile, rb_intern("basename"), 2, filename, extname);
    const char* cExtName = StringValuePtr(extname);
    size_t len = strlen(cExtName);
    VALUE queue = rb_cvar_get(rb_class_of(loader), rb_intern("@@_queue")); 
    volatile VALUE proc = Qnil;
    volatile VALUE type_s = rb_funcall(type, rb_intern("to_s"), 0);
    const char* cType = StringValuePtr(type_s);
    if ((strncmp(cType, "image", len) == 0)) {
        proc = rb_proc_new(load_image, filename);
    } else if ((strncmp(cType, "bgm", len) == 0)) {
        proc = rb_proc_new(load_music, filename);
    } else if ((strncmp(cType, "se", len) == 0)) {
        proc = rb_proc_new(load_effect, filename);
    }
    if ( (strncmp(cExtName, ".png", len) == 0 ) || (strncmp(cExtName, ".gif", len) == 0 )) {
        if (NIL_P(proc)) proc = rb_proc_new(load_image, filename);
        rb_funcall(queue, rb_intern("push"), 1, rb_ary_new3(3, proc, rb_funcall(access_key, rb_intern("to_s"), 0), rb_str_new2("image") ));
    } else if ( (strncmp(cExtName, ".wav", len) == 0 ) || 
        (strncmp(cExtName, ".mp3", len) == 0 ) || (strncmp(cExtName, ".ogg", len) == 0 )) {
        if (NIL_P(proc)) proc = rb_proc_new(load_effect, filename);
        rb_funcall(queue, rb_intern("push"), 1, rb_ary_new3(3, proc, rb_funcall(access_key, rb_intern("to_s"), 0), rb_str_new2(cType)));
    }
    LoaderData* loaderData;Data_Get_Struct(loader, LoaderData, loaderData);
    loaderData->complete = FALSE;
}

static void EachPushLoader(VALUE self, VALUE ary, VALUE type) {
    int len = RARRAY_LENINT(ary);
    int i; for (i = 0; i < len; ++i) {
        volatile VALUE val = RARRAY_PTR(ary)[i];
        if (RB_TYPE_P(val, T_HASH)) {
            volatile VALUE val_ary = rb_funcall(val, rb_intern("to_a"), 0);
            int valLen = RARRAY_LENINT(val_ary);
            int j; for (j = 0; j < valLen; ++j) {
                volatile VALUE pair = RARRAY_PTR(val_ary)[j];
                PushLoadTask(self, RARRAY_PTR(pair)[0], RARRAY_PTR(pair)[1], type);
            }
        } else if (RB_TYPE_P(val, T_STRING)) {
            PushLoadTask(self, Qnil, val, type);
        }
    }
}

static VALUE loader_set(int argc, VALUE argv[], VALUE self) {
    volatile VALUE ary = Qundef;
    rb_scan_args(argc, argv, "*", &ary);
    VALUE first = rb_ary_entry(ary, 0);
    if (RB_TYPE_P(first, T_HASH)) {
        volatile VALUE first_ary = rb_funcall(first, rb_intern("to_a"), 0);
        int i, len = RARRAY_LENINT(first_ary);
        for (i = 0; i < len; ++i) {
            VALUE pair = RARRAY_PTR(first_ary)[i];
            ary = RARRAY_PTR(pair)[1];
            if (!RB_TYPE_P(ary, T_ARRAY)) ary = rb_funcall(rb_cArray, rb_intern("[]"), 1, ary);
            EachPushLoader(self, ary, RARRAY_PTR(pair)[0]);
        }
    } else {
        ary = rb_funcall(ary, rb_intern("flatten"), 0);
        EachPushLoader(self, ary, Qnil);
    }
    return Qnil;
}

static VALUE CreateImageFromLoader(SDL_Surface * surface, int windowID) {
    SDL_Renderer* renderer = (windowID == 0) ? systemData->renderer : GetWindowRenderer(NULL, windowID);
    SDL_Color color;
    SDL_Texture* texture = CreateTextureFromSurface(surface, renderer, &color);
    VALUE image = rb_path2class("SDLUdon::Image");
    volatile VALUE image_instance = rb_obj_alloc(image);
    ImageData *imageData;Data_Get_Struct(image_instance, ImageData, imageData);
    imageData->colorKey = color;
    imageData->rect = SDL_malloc(sizeof(SDL_Rect));
    imageData->rect->x = imageData->rect->y = 0;
    SDL_QueryTexture(texture, NULL, NULL, &imageData->rect->w, &imageData->rect->h);
    imageData->texture = texture;
    imageData->attach_id = windowID + 1;
    return image_instance;
}
static VALUE CreateMusicFromLoader(const char* name, const Uint8* buff) {
    VALUE class_music = rb_path2class("SDLUdon::Sound::Music");
    volatile VALUE music = rb_obj_alloc(class_music);
    MusicData* musicData; Data_Get_Struct(music, MusicData, musicData);
    musicData->volume = 0.5; 
    musicData->music = CreateBGMFromMEMAtCache(name, buff);
    return music;
}

static VALUE CreateEffectFromLoader(const char* name, const Uint8* buff) {
    VALUE class_effect = rb_path2class("SDLUdon::Sound::Effect");
    volatile VALUE effect = rb_obj_alloc(class_effect);
    EffectData* effectData; Data_Get_Struct(effect, EffectData, effectData);
    effectData->channel  = -1;
    effectData->volume = 0.5; 
    effectData->chunk = CreateSEFromMEMAtCache(name, buff);
    return effect;
}

static int LoaderGetStoreHash(st_data_t k, st_data_t v, st_data_t result_hash) {
    const char* keyStr = (const char*)v;
    rb_hash_aset(result_hash, ID2SYM(rb_intern(keyStr)), (VALUE)v);
    return ST_CONTINUE;
}
static VALUE loader_get(int argc, VALUE argv[], VALUE self) {
    LoaderData *loader; Data_Get_Struct(self, LoaderData, loader);
    VALUE type, key;
    volatile VALUE opt;
    rb_scan_args(argc, argv, "12", &type, &key, &opt);
    if (NIL_P(opt)) opt = rb_hash_new();
    st_data_t data;
    volatile VALUE type_s = rb_funcall(type, rb_intern("to_s"), 0);
    const char* cType = StringValuePtr( type_s );
    int success = st_lookup(loader->table, (st_data_t)cType, &data);

    if (!success) return Qnil;
    st_table* typeTable = (st_table*)data;
    if (NIL_P(key)) {
        volatile VALUE result_hash = rb_hash_new();
        st_foreach_safe(typeTable, LoaderGetStoreHash, (st_data_t)result_hash);
        return result_hash;
    } else {
        volatile VALUE key_s = rb_funcall(key, rb_intern("to_s"), 0);
        const char* cKey = StringValuePtr(key_s);
        success = st_lookup(typeTable, (st_data_t)cKey, &data);
        if (!success) return Qnil;
        int typeLen = strlen(cType);
        if (strncmp(cType, "image", typeLen) == 0) {
            SDL_Surface* surface = (SDL_Surface*)data;
            int windowID = 0;
            if (RB_TYPE_P(opt, T_HASH)) {
                VALUE id = rb_hash_lookup(opt, ID2SYM(rb_intern("window_id")));
                windowID = NUM2INT(rb_funcall(id, rb_intern("to_i"), 0));
            } else {
                windowID = NUM2INT(opt);
            }
            return CreateImageFromLoader(surface, windowID);
        } else if (strncmp(cType, "bgm", typeLen) == 0) {
            return CreateMusicFromLoader(cKey, (const Uint8*)data);
        } else if (strncmp(cType, "se", typeLen) == 0) {
            return CreateEffectFromLoader(cKey, (const Uint8*)data);
        }
    }
    return Qnil;
}

static VALUE loader_get_at(int argc, VALUE argv[], VALUE self) {
     return loader_get(argc, argv, self);
}

static VALUE loader_is_complete(VALUE self) {
    LoaderData* loaderData;Data_Get_Struct(self, LoaderData, loaderData);
    if (loaderData->error) {
        rb_raise(rb_eException, "load error!");
    }
    return loaderData->complete ? Qtrue : Qfalse;
}


void Init_manager(VALUE parent_module) {
    VALUE manager = rb_define_module_under(parent_module, "Manager");
    rb_define_method(manager, "init_manage", module_manager_init_manage, 0);
    rb_define_method(manager, "store", module_manager_store, -1);
    rb_define_method(manager, "fetch", module_manager_fetch, -1);
    rb_define_method(manager, "loading?", module_manager_is_loading, 0);
    rb_define_method(manager, "loaded?", module_manager_is_loaded, 0);

    VALUE loader = rb_define_class_under(manager, "Loader", rb_cObject);
    volatile VALUE queue = rb_class_new_instance(0, NULL, rb_path2class("Queue"));
    rb_cvar_set(loader, rb_intern("@@_queue"), queue);
    rb_cvar_set(loader, rb_intern("@@_mutex"), rb_mutex_new());
    rb_define_alloc_func(loader, loader_alloc);
    rb_define_private_method(loader, "initialize", loader_initialize, 0);
    rb_define_method(loader, "set", loader_set, -1);
    rb_define_method(loader, "[]", loader_get_at, -1);
    rb_define_method(loader, "get", loader_get, -1);
    rb_define_method(loader, "complete?", loader_is_complete, 0);
}
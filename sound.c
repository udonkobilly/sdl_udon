#include "sdl_udon_private.h"

static SoundEffectCache *mEffectCache = NULL;
static SoundMusicCache *mMusicCache = NULL;

void FreeSoundCache() {
    Mix_HaltMusic();
    Mix_HaltChannel(-1);
    SoundEffectCache *effectNode = mEffectCache;
    while (effectNode != NULL) {
        Mix_FreeChunk(effectNode->chunk);
        effectNode = effectNode->next;
    }
    SoundMusicCache *musicNode = mMusicCache;
    while (musicNode != NULL) {
        Mix_FreeMusic(musicNode->music);
        musicNode = musicNode->next;
    }
}

Mix_Music* CreateMusicAtCache(const char* fileName) {
    SoundMusicCache * cache = mMusicCache;
    SoundMusicCache * prev_cache = NULL;
    Mix_Music *music = NULL;
    while (cache != NULL) {
        size_t size = strlen(cache->name);
        if (strncmp(cache->name, fileName, size ) == 0) {
            music = cache->music;
            break;
        }
        prev_cache = cache;
        cache = cache->next;
    }
    if (music == NULL) {
        cache = mMusicCache;
        SoundMusicCache *regist_cache;
        if (cache == NULL)  {
            mMusicCache = malloc(sizeof(SoundMusicCache));
            regist_cache = mMusicCache;
            prev_cache  = regist_cache;
        } else {
            prev_cache->next = malloc(sizeof(SoundMusicCache));
            regist_cache = prev_cache->next;
        }
        regist_cache->music = Mix_LoadMUS(fileName) ;
        if (regist_cache->music == NULL) {
            prev_cache->next = NULL;
            SDL_LOG_ABORT();
        }
        music = regist_cache->music;
        size_t size = strlen(fileName) + 1;
        regist_cache->name = malloc(sizeof(char) * size);
        snprintf(regist_cache->name, size, "%s", fileName);
        regist_cache->next = NULL;
    }
    return music;
}

Mix_Music* CreateBGMFromMEMAtCache(const char* fileName, const Uint8* mem) {
    SoundMusicCache * cache = mMusicCache;
    SoundMusicCache * prev_cache = NULL;
    Mix_Music *music = NULL;
    while (cache != NULL) {
        size_t size = strlen(cache->name);
        if (strncmp(cache->name, fileName, size ) == 0) {
            music = cache->music;
            break;
        }
        prev_cache = cache;
        cache = cache->next;
    }
    if (music == NULL) {
        cache = mMusicCache;
        SoundMusicCache *regist_cache;
        if (cache == NULL)  {
            mMusicCache = malloc(sizeof(SoundMusicCache));
            regist_cache = mMusicCache;
            prev_cache  = regist_cache;
        } else {
            prev_cache->next = malloc(sizeof(SoundMusicCache));
            regist_cache = prev_cache->next;
        }
        Uint32 len = (mem[0] << 24) + (mem[1] << 16) + (mem[2] << 8) + mem[3];
        SDL_RWops* rw = SDL_RWFromConstMem(mem+4, len);
        regist_cache->music = Mix_LoadMUS_RW(rw, 1);
        if (regist_cache->music == NULL) {
            prev_cache->next = NULL;
            SDL_LOG_ABORT();
        }
        music = regist_cache->music;
        size_t size = strlen(fileName) + 1;
        regist_cache->name = malloc(sizeof(char) * size);
        snprintf(regist_cache->name, size, "%s", fileName);
        regist_cache->next = NULL;
    }
    return music;
}

Mix_Chunk* CreateSEAtCache(const char* fileName) {
    SoundEffectCache * cache = mEffectCache;
    SoundEffectCache * prev_cache = NULL;
    Mix_Chunk *chunk = NULL;
    while (cache != NULL) {
        size_t size = strlen(cache->name);
        if (strncmp(cache->name, fileName, size ) == 0) {
            chunk = cache->chunk;
            break;
        }
        prev_cache = cache;
        cache = cache->next;
    }
    if (chunk == NULL) {
        cache = mEffectCache;
        SoundEffectCache *regist_cache;
        if (cache == NULL)  {
            mEffectCache = malloc(sizeof(SoundEffectCache));
            regist_cache = mEffectCache;
        } else {
            prev_cache->next = malloc(sizeof(SoundEffectCache));
            regist_cache = mEffectCache;
        }
        regist_cache->chunk = Mix_LoadWAV(fileName);
        if (regist_cache->chunk == NULL) {
            VALUE argv[] = { rb_str_new2(SDL_GetError())  };
            rb_f_abort(1, argv);
        }
        size_t size = strlen(fileName) + 1;
        regist_cache->name = malloc(sizeof(char) * size);
        snprintf(regist_cache->name, size, "%s", fileName);
        regist_cache->next = NULL;
        chunk = regist_cache->chunk;
    } else {
        chunk  = chunk;
    }
    if (chunk == NULL) rb_f_abort(1, (VALUE[]){ rb_str_new2(SDL_GetError()) });
    return chunk;
}

Mix_Chunk* CreateSEFromMEMAtCache(const char* name, const Uint8* mem) {
    SoundEffectCache * cache = mEffectCache;
    SoundEffectCache * prev_cache = NULL;
    Mix_Chunk *chunk = NULL;
    while (cache != NULL) {
        size_t size = strlen(cache->name);
        if (strncmp(cache->name, name, size ) == 0) {
            chunk = cache->chunk;
            break;
        }
        prev_cache = cache;
        cache = cache->next;
    }
    if (chunk == NULL) {
        cache = mEffectCache;
        SoundEffectCache *regist_cache;
        if (cache == NULL)  {
            mEffectCache = malloc(sizeof(SoundEffectCache));
            regist_cache = mEffectCache;
        } else {
            prev_cache->next = malloc(sizeof(SoundEffectCache));
            regist_cache = mEffectCache;
        }
        regist_cache->chunk = Mix_QuickLoad_WAV((Uint8*)mem);
        if (regist_cache->chunk == NULL) {
            VALUE argv[] = { rb_str_new2(SDL_GetError())  };
            rb_f_abort(1, argv);
        }
        size_t size = strlen(name) + 1;
        regist_cache->name = malloc(sizeof(char) * size);
        snprintf(regist_cache->name, size, "%s", name);
        regist_cache->next = NULL;
        chunk = regist_cache->chunk;
    } else {
        chunk  = chunk;
    }
    if (chunk == NULL) rb_f_abort(1, (VALUE[]){ rb_str_new2(SDL_GetError()) });
    return chunk;
}


void SetUpSound() { Mix_AllocateChannels(16); }

static VALUE music_alloc(VALUE self) {
    MusicData*music = ALLOC(MusicData);
    return Data_Wrap_Struct(self, 0, -1, music);
}

static VALUE music_initialize(VALUE self, VALUE file_name) {
    MusicData* musicData; Data_Get_Struct(self, MusicData, musicData);
    musicData->volume = 0.5;
    musicData->music = CreateMusicAtCache(StringValuePtr(file_name));
    return Qnil;
}

static VALUE music_class_load(VALUE klass, VALUE filename) {
   volatile VALUE music_instance = Qundef;
    VALUE ext_name = rb_funcall(rb_cFile, rb_intern("extname"), 1, rb_funcall(filename, rb_intern("to_s"), 0));
    if (rb_funcall(ext_name, rb_intern("empty?"), 0) == Qtrue ) {
        VALUE game = rb_path2class("SDLUdon::Game");
        music_instance = rb_funcall(game, rb_intern("fetch"), 2, rb_str_new2("bgm"), filename);
        if (NIL_P(music_instance)) rb_raise(rb_eLoadError, "%s", "music not found...!");
    } else {
        music_instance = rb_class_new_instance(1, &filename, klass);
    }
    return music_instance;
} 

static VALUE music_get_volume(VALUE self) {
    MusicData* musicData; Data_Get_Struct(self, MusicData, musicData);
    return rb_float_new(musicData->volume);
}

static VALUE music_set_volume(VALUE self, VALUE volume) {
    MusicData* musicData; Data_Get_Struct(self, MusicData, musicData);
    musicData->volume =NUM2DBL(volume);
    int cVolume = MIX_MAX_VOLUME * musicData->volume;
    Mix_VolumeMusic(cVolume);
    return Qnil;
}

static VALUE music_play(int argc, VALUE argv[], VALUE self) {
    int cVolume, cFade, cLoop, cPosition;
    volatile VALUE option_or_volume, fade, position, loop;
    rb_scan_args(argc, argv, "04", &option_or_volume, &fade, &position, &loop);
    if (rb_type_p(option_or_volume, T_HASH)) {
        fade = rb_hash_lookup(option_or_volume, ID2SYM(rb_intern("fade")));
        position = rb_hash_lookup(option_or_volume, ID2SYM(rb_intern("position")));
        loop = rb_hash_lookup(option_or_volume, ID2SYM(rb_intern("loop")));
        option_or_volume = rb_hash_lookup(option_or_volume, ID2SYM(rb_intern("volume")));
    }
    MusicData* musicData; Data_Get_Struct(self, MusicData, musicData);
    if (!NIL_P(option_or_volume)) musicData->volume = NUM2DBL(option_or_volume);
    cVolume = MIX_MAX_VOLUME * musicData->volume;
    Mix_VolumeMusic(cVolume);
    cFade = NIL_P(fade) ? 0 : (NUM2DBL(fade) * 1000);
    cLoop = NIL_P(loop) ? -1 : NUM2INT(loop);
    cPosition = NIL_P(position) ? 0 : NUM2DBL(position);
    return Mix_FadeInMusicPos(musicData->music, cLoop, cFade, cPosition) ? self : Qnil;
}

static VALUE music_rewind(VALUE self) { Mix_RewindMusic(); return self; }

static VALUE music_set_position(VALUE self, VALUE sec) {
    double_t cSec = NUM2DBL(sec);
    music_rewind(self);
    Mix_SetMusicPosition(cSec);
    return self;
} 

static VALUE music_is_playing(VALUE self) { return (Mix_PlayingMusic() == 1) ? Qtrue : Qfalse; }
static VALUE music_is_paused(VALUE self) { return (Mix_PlayingMusic() == 0 || Mix_PausedMusic() == 1) ? Qtrue : Qfalse; }
static VALUE music_is_fading(VALUE self) { return (Mix_FadingMusic() != MIX_NO_FADING) ? Qtrue : Qfalse; }

static VALUE music_stop(VALUE self) {
    Mix_HaltMusic();
    return Qnil;
}

static VALUE music_pause(VALUE self) {
    Mix_PauseMusic();
    return Qnil;
}

static VALUE music_resume(VALUE self) {
    Mix_ResumeMusic();
    return Qnil;
}

static VALUE music_toggle(VALUE self) {
    (Mix_PausedMusic() == 1) ? Mix_ResumeMusic() : Mix_PauseMusic();
    return Qnil;
}

static VALUE music_fade_out(VALUE self, VALUE num) {
    Mix_FadeOutMusic( NUM2DBL(num)*1000);
    return Qnil; 
}

static VALUE effect_alloc(VALUE self) {
    EffectData*effect = ALLOC(EffectData);
    return Data_Wrap_Struct(self, 0, -1, effect);
}

static VALUE effect_initialize(VALUE self, VALUE file_name) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    effectData->channel  = -1;
    effectData->volume = 0.5;
    effectData->chunk = CreateSEAtCache(StringValuePtr(file_name));
    return Qnil;
}

static VALUE effect_class_load(VALUE klass, VALUE filename) {
   volatile VALUE effect_instance = Qundef;
    VALUE ext_name = rb_funcall(rb_cFile, rb_intern("extname"), 1, rb_funcall(filename, rb_intern("to_s"), 0));
    if (rb_funcall(ext_name, rb_intern("empty?"), 0) == Qtrue ) {
        VALUE game = rb_path2class("SDLUdon::Game");
        effect_instance = rb_funcall(game, rb_intern("fetch"), 2, rb_str_new2("se"), filename);
        if (NIL_P(effect_instance)) rb_raise(rb_eLoadError, "%s", "effect not found...!");
    } else {
        effect_instance = rb_class_new_instance(1, &filename, klass);
    }
    return effect_instance;
}

static VALUE effect_play(int argc, VALUE argv[], VALUE self) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    int cVolume, cFade, cPan, cLoop, cTick;
    volatile VALUE option_or_volume, fade, pan, loop, tick;
    rb_scan_args(argc, argv, "05", &option_or_volume, &fade, &pan, &loop, &tick);
    if (rb_type_p(option_or_volume, T_HASH)) {
        fade = rb_hash_lookup(option_or_volume, ID2SYM(rb_intern("fade")));
        pan = rb_hash_lookup(option_or_volume, ID2SYM(rb_intern("pan")));
        loop = rb_hash_lookup(option_or_volume, ID2SYM(rb_intern("loop")));
        tick = rb_hash_lookup(option_or_volume, ID2SYM(rb_intern("time")));
        option_or_volume = rb_hash_lookup(option_or_volume, ID2SYM(rb_intern("volume")));
    }
    if (!NIL_P(option_or_volume)) effectData->volume = NUM2DBL(option_or_volume);
    cVolume = MIX_MAX_VOLUME * effectData->volume;
    cFade = NIL_P(fade) ? 0 : (NUM2DBL(fade) * 1000);
    cPan = NIL_P(pan) ? 127 : 254 * NUM2DBL(pan);
    cLoop = NIL_P(loop) ? 0 : NUM2INT(loop);
    cTick = NIL_P(tick) ? -1 : (NUM2DBL(tick) * 1000);
    Mix_VolumeChunk(effectData->chunk, cVolume);
    effectData->channel = Mix_FadeInChannelTimed(-1, effectData->chunk, cLoop, cFade, cTick);
    Mix_SetPanning(effectData->channel, cPan, 254 - cPan);
    return effectData->channel ? self : Qnil;
}

static VALUE effect_get_volume(VALUE self) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    return rb_float_new(effectData->volume);
}

static VALUE effect_set_volume(VALUE self, VALUE volume) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    effectData->volume = NUM2DBL(volume);
    int cVolume = MIX_MAX_VOLUME * effectData->volume;
    Mix_VolumeChunk(effectData->chunk, cVolume);
    return Qnil;
}

BOOL IsUnattachedChunk(EffectData* data) {
    return ( data->channel == -1 || Mix_GetChunk(data->channel) != data->chunk);
}

static VALUE effect_is_playing(VALUE self) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    if ( IsUnattachedChunk(effectData)) return Qfalse;
    return (Mix_Playing(effectData->channel) == 1) ? Qtrue : Qfalse;
}

static VALUE effect_is_paused(VALUE self) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    if ( IsUnattachedChunk(effectData)) return Qtrue;
    return (Mix_Playing(effectData->channel) == 0 || Mix_Paused(effectData->channel) == 1) ? Qtrue : Qfalse;
}

static VALUE effect_is_fading(VALUE self) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    if ( IsUnattachedChunk(effectData)) return Qnil;
    return (Mix_FadingChannel(effectData->channel) != MIX_NO_FADING) ? Qtrue : Qfalse;
}

static VALUE sound_module_effect_size() {
    int size = 0;
    int i;for(i = 0; i < 16; ++i) {
        if (Mix_GetChunk(i) != NULL) { ++size; }
    }
    return rb_int_new(size);
}

static VALUE effect_fade_out(VALUE self, VALUE ticks) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    if ( IsUnattachedChunk(effectData)) return Qnil;
    Mix_FadeOutChannel(effectData->channel, NUM2DBL(ticks) * 1000);
    return Qnil;
}

static VALUE effect_stop(int argc, VALUE argv[], VALUE self) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    if ( IsUnattachedChunk(effectData)) return Qnil;
    VALUE ticks;
    rb_scan_args(argc, argv, "01", &ticks);
    Mix_ExpireChannel(effectData->channel, NIL_P(ticks) ? -1 : (NUM2DBL(ticks) * 1000) );
    return Qnil;
}

static VALUE effect_pause(VALUE self) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    if ( IsUnattachedChunk(effectData)) return Qnil;    
    Mix_Pause(effectData->channel);
    return Qnil;
}

static VALUE effect_resume(VALUE self) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    if ( IsUnattachedChunk(effectData)) return Qnil;    
    Mix_Resume(effectData->channel);
    return Qnil;
}

static VALUE effect_toggle(VALUE self) {
    EffectData* effectData; Data_Get_Struct(self, EffectData, effectData);
    if ( IsUnattachedChunk(effectData)) return Qnil;    
    (Mix_Paused(effectData->channel) == 1) ? Mix_Resume(effectData->channel) : Mix_Pause(effectData->channel);
    return Qnil;
}

static VALUE sound_module_play_bgm(int argc, VALUE argv[], VALUE module) {
    volatile VALUE file_name, opt;
    if (rb_scan_args(argc, argv, "11", &file_name, &opt) == 1) {
        int cVolume, cFade, cLoop, cPosition;
        volatile VALUE volume, fade, position, loop;

        fade = rb_hash_lookup(fade, ID2SYM(rb_intern("fade")));
        position = rb_hash_lookup(position, ID2SYM(rb_intern("position")));
        loop = rb_hash_lookup(loop, ID2SYM(rb_intern("loop")));
        volume = rb_hash_lookup(volume, ID2SYM(rb_intern("volume")));
        cFade = NIL_P(fade) ? 0 : (NUM2DBL(fade) * 1000);
        cLoop = NIL_P(loop) ? -1 : NUM2INT(loop);
        cPosition = NIL_P(position) ? 0 : NUM2DBL(position);
        cVolume = NIL_P(volume) ? (MIX_MAX_VOLUME * 0.5) : ( MIX_MAX_VOLUME * NUM2DBL(volume) );
        Mix_VolumeMusic(cVolume);
        return Mix_FadeInMusicPos(CreateMusicAtCache(StringValuePtr(file_name)), cLoop, cFade, cPosition) ? module : Qnil;
    } else {
        return Mix_PlayMusic(CreateMusicAtCache(StringValuePtr(file_name)), -1) ? module : Qnil;
    }
}

static VALUE sound_module_play_se(int argc, VALUE argv[], VALUE module) {
    volatile VALUE file_name, opt;
    if (rb_scan_args(argc, argv, "11", &file_name, &opt) == 1) {
        int cVolume, cFade, cPan, cLoop, cTick;
        volatile VALUE volume, fade, pan, loop, tick;
        fade = rb_hash_lookup(volume, ID2SYM(rb_intern("fade")));
        pan = rb_hash_lookup(volume, ID2SYM(rb_intern("pan")));
        loop = rb_hash_lookup(volume, ID2SYM(rb_intern("loop")));
        tick = rb_hash_lookup(volume, ID2SYM(rb_intern("time")));
        volume = rb_hash_lookup(volume, ID2SYM(rb_intern("volume")));
        cVolume = NIL_P(volume) ? (MIX_MAX_VOLUME * 0.5) : ( MIX_MAX_VOLUME * NUM2DBL(volume) );
        cFade = NIL_P(fade) ? 0 : (NUM2DBL(fade) * 1000);
        cPan = NIL_P(pan) ? 127 : 254 * NUM2DBL(pan);
        cLoop = NIL_P(loop) ? 0 : NUM2INT(loop);
        cTick = NIL_P(tick) ? -1 : (NUM2DBL(tick) * 1000);
        Mix_Chunk *chunk = CreateSEAtCache(StringValuePtr(file_name));
        Mix_VolumeChunk(chunk, cVolume);
        int channel = Mix_FadeInChannelTimed(-1, chunk, cLoop, cFade, cTick);
        if (!channel) {
            return Qnil;
        } else {
            Mix_SetPanning(channel, cPan, 254 - cPan);
            return module;
        }
    } else {
        return Mix_PlayChannel(-1, CreateSEAtCache(StringValuePtr(file_name)), 0) ? module : Qnil;
    }
    return Qnil;
}

static VALUE sound_module_load_bgm(VALUE module, VALUE file_name) {
     VALUE instance = rb_funcall(rb_path2class("SDLUdon::Sound::Music"), rb_intern("load"), 1, file_name);
     rb_extend_object(instance, module);
     return instance;
}

static VALUE sound_module_load_se(VALUE module, VALUE file_name) {
     VALUE instance = rb_funcall(rb_path2class("SDLUdon::Sound::Effect"), rb_intern("load"), 1, file_name);
      rb_extend_object(instance, module);
     return instance;
}
 
static VALUE sound_module_stop(VALUE module) {
    Mix_HaltMusic();
    Mix_HaltChannel(-1);
    return Qnil;
}

static VALUE sound_module_stop_bgm(VALUE module) {
    Mix_HaltMusic();
    return Qnil;
}

static VALUE sound_module_stop_se(VALUE module) {
    Mix_HaltChannel(-1);
    return Qnil;
}

static VALUE sound_module_pause(VALUE module) {
    Mix_PauseMusic(); Mix_Pause(-1);
    return Qnil;
}

static VALUE sound_module_pause_bgm(VALUE module) { Mix_PauseMusic(); return Qnil; }
static VALUE sound_module_pause_se(VALUE module) { Mix_Pause(-1); return Qnil; }

static VALUE sound_module_resume(VALUE module) {
    Mix_ResumeMusic(); Mix_Resume(-1);
    return Qnil;
}

static VALUE sound_module_resume_bgm(VALUE module) { Mix_ResumeMusic(); return Qnil; }
static VALUE sound_module_resume_se(VALUE module) { Mix_Resume(-1); return Qnil; }

static VALUE sound_module_toggle(VALUE module) {
    (Mix_PausedMusic() == 1) ? Mix_ResumeMusic() : Mix_PauseMusic();
    int num_channels = 16;
    int i;for (i = 0; i < num_channels; ++i) {
        (Mix_Paused(i) == 1) ? Mix_Resume(i) : Mix_Pause(i);
    }
    return Qnil;
}


static VALUE sound_module_toggle_bgm(VALUE module) {
    (Mix_PausedMusic() == 1) ? Mix_ResumeMusic() : Mix_PauseMusic();
    return Qnil;
}

static VALUE sound_module_toggle_se(VALUE module) {
    int num_channels = 16;
    int i;for (i = 0; i < num_channels; ++i) {
        (Mix_Paused(i) == 1) ? Mix_Resume(i) : Mix_Pause(i);
    }
    return Qnil;
}

static VALUE sound_module_se_is_playing(VALUE self) {
    return (Mix_Playing(-1) == 1) ? Qtrue : Qfalse;
}

static VALUE sound_module_bgm_is_playing(VALUE self) {
    return (Mix_PlayingMusic() == 1) ? Qtrue : Qfalse;
}




void Init_sound(VALUE parent) {
    VALUE sound_module = rb_define_module_under(parent, "Sound");
    rb_define_singleton_method(sound_module, "load_bgm", sound_module_load_bgm, 1);
    rb_define_singleton_method(sound_module, "load_se", sound_module_load_se, 1);
    rb_define_singleton_method(sound_module, "play_bgm", sound_module_play_bgm, -1);
    rb_define_singleton_method(sound_module, "play_se", sound_module_play_se, -1);
    rb_define_singleton_method(sound_module, "stop", sound_module_stop, 0);
    rb_define_singleton_method(sound_module, "stop_bgm", sound_module_stop_bgm, 0);
    rb_define_singleton_method(sound_module, "stop_se", sound_module_stop_se, 0);
    rb_define_singleton_method(sound_module, "pause", sound_module_pause, 0);
    rb_define_singleton_method(sound_module, "pause_bgm", sound_module_pause_bgm, 0);
    rb_define_singleton_method(sound_module, "pause_se", sound_module_pause_se, 0);
    rb_define_singleton_method(sound_module, "resume", sound_module_resume, 0);
    rb_define_singleton_method(sound_module, "resume_bgm", sound_module_resume_bgm, 0);
    rb_define_singleton_method(sound_module, "resume_se", sound_module_resume_se, 0);
    rb_define_singleton_method(sound_module, "toggle", sound_module_toggle, 0);
    rb_define_singleton_method(sound_module, "toggle_bgm", sound_module_toggle_bgm, 0);
    rb_define_singleton_method(sound_module, "toggle_se", sound_module_toggle_se, 0);
    rb_define_singleton_method(sound_module, "se_playing?", sound_module_se_is_playing, 0);
    rb_define_singleton_method(sound_module, "bgm_playing?", sound_module_bgm_is_playing, 0);
    rb_define_singleton_method(sound_module, "active_se_size", sound_module_effect_size, 0);

    VALUE music_class = rb_define_class_under(sound_module, "Music", rb_cObject);
    rb_define_singleton_method(music_class,"load", music_class_load, 1);
    rb_define_alloc_func(music_class, music_alloc);
    rb_define_private_method(music_class, "initialize", music_initialize, 1);
    rb_define_method(music_class, "play", music_play, -1);
    rb_define_method(music_class, "stop", music_stop, 0);
    rb_define_method(music_class, "pause", music_pause, 0);
    rb_define_method(music_class, "resume", music_resume, 0);
    rb_define_method(music_class, "toggle", music_toggle, 0);
    rb_define_method(music_class, "volume", music_get_volume, 0);
    rb_define_method(music_class, "volume=", music_set_volume, 1);
    rb_define_method(music_class, "position=", music_set_position, 1);
    rb_define_method(music_class, "fade_out", music_fade_out, 1);
    rb_define_method(music_class, "playing?", music_is_playing, 0);
    rb_define_method(music_class, "paused?", music_is_paused, 0);
    rb_define_method(music_class, "fading?", music_is_fading, 0);
    rb_define_method(music_class, "rewind", music_rewind, 0);

    VALUE effect_class = rb_define_class_under(sound_module, "Effect", rb_cObject);
    rb_define_singleton_method(effect_class,"load", effect_class_load, 1);
    rb_define_alloc_func(effect_class, effect_alloc);
    rb_define_private_method(effect_class, "initialize", effect_initialize, 1);
    rb_define_method(effect_class, "play", effect_play, -1);
    rb_define_method(effect_class, "fade_out", effect_fade_out, 1);
    rb_define_method(effect_class, "stop", effect_stop, -1);
    rb_define_method(effect_class, "pause", effect_pause, 0);
    rb_define_method(effect_class, "resume", effect_resume, 0);
    rb_define_method(effect_class, "toggle", effect_toggle, 0);
    rb_define_method(effect_class, "volume", effect_get_volume, 0);
    rb_define_method(effect_class, "volume=", effect_set_volume, 1);
    rb_define_method(effect_class, "playing?", effect_is_playing, 0);
    rb_define_method(effect_class, "paused?", effect_is_paused, 0);
    rb_define_method(effect_class, "fading?", effect_is_fading, 0);
}
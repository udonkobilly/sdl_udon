#include "sdl_udon_private.h"

double_t easing_none(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    return initNum + nowTime / duration * changeNum;
}

double_t easing_in_quad(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate  = nowTime / duration;
    return changeNum * nowTimeRate * nowTimeRate + initNum;
}

double_t easing_out_quad(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate  = nowTime / duration;
    return -changeNum * nowTimeRate * (nowTimeRate - 2) + initNum;
}

double_t easing_in_out_quad(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate  = nowTime / (duration / 2);
    if (nowTimeRate < 1) { return changeNum / 2 * nowTimeRate * nowTimeRate + initNum; }
    nowTimeRate--;
    return -changeNum/2 * (nowTimeRate*(nowTimeRate-2)-1) + initNum;
}

double_t easing_out_in_quad(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    if (nowTime < duration/2) { return easing_out_quad(nowTime*2, initNum, changeNum/2, duration); } 
    return easing_in_quad((nowTime*2)-duration, initNum+changeNum/2, changeNum/2, duration);
}

double_t easing_cubic(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate = nowTime / duration;
    return changeNum*nowTimeRate*nowTimeRate*nowTimeRate+initNum;
 }

double_t easing_cubic_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate = nowTime / duration;
    nowTimeRate--;
    return changeNum*(nowTimeRate*nowTimeRate*nowTimeRate + 1) + initNum;
}

double_t easing_cubic_in_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate = nowTime / (duration/2);
    if (nowTimeRate < 1) { return changeNum/2*nowTimeRate*nowTimeRate*nowTimeRate + initNum; }
    nowTimeRate -= 2;
    return changeNum/2*(nowTimeRate*nowTimeRate*nowTimeRate + 2) + initNum;
}

double_t easing_cubic_out_in(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    if (nowTime < duration/2) { return easing_cubic_out(nowTime*2, initNum, changeNum/2, duration); } 
    return easing_cubic((nowTime*2)-duration, initNum+changeNum/2, changeNum/2, duration);
}

double_t easing_quart(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate = nowTime / duration;
    return changeNum*nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate + initNum;
}

double_t easing_quart_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate = nowTime / duration;
    nowTimeRate--;
    return -changeNum * (nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate - 1) + initNum;
}

double_t easing_quart_in_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate = nowTime / (duration/2);
    if (nowTimeRate < 1) { return changeNum/2*nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate + initNum; }
    nowTimeRate -= 2;
    return -changeNum/2 * (nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate - 2) + initNum;
}

double_t easing_quart_out_in(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    if (nowTime < duration/2) { return easing_quart_out(nowTime*2, initNum, changeNum/2, duration); } 
    return easing_quart((nowTime*2)-duration, initNum+changeNum/2, changeNum/2, duration);
}

double_t easing_quint(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate = nowTime / duration;
    return changeNum*nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate + initNum;
}

double_t easing_quint_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate = nowTime / duration;
    nowTimeRate--;
    return changeNum*(nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate + 1) + initNum;
}

double_t easing_quint_in_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    double_t nowTimeRate = nowTime / (duration/2);
    if (nowTimeRate < 1) return changeNum/2*nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate + initNum;
    nowTimeRate -= 2;
    return changeNum/2*(nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate*nowTimeRate + 2) + initNum;
}

double_t easing_quint_out_in(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    if (nowTime < duration/2) { return easing_quint_out(nowTime*2, initNum, changeNum/2, duration); } 
    return easing_quint((nowTime*2)-duration, initNum+changeNum/2, changeNum/2, duration);
}

double_t easing_sin(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    return -changeNum * cos(nowTime/duration * (MATH_PI/2)) + changeNum + initNum;
}

double_t easing_sin_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    return changeNum * sin(nowTime/duration * (MATH_PI/2)) + initNum;
}

double_t easing_sin_in_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    return -changeNum/2 * (cos(MATH_PI*nowTime/duration) - 1) + initNum;
}

double_t easing_sin_out_in(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    if (nowTime < duration / 2) { return easing_sin_out(nowTime*2, initNum, changeNum/2,duration); }
    return easing_sin(nowTime*2-duration, initNum+changeNum/2, changeNum/2, duration);
};
double_t easing_expo(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    return changeNum * pow( 2, 10 * (nowTime/duration - 1) ) + initNum;
};

double_t easing_expo_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    return changeNum * ( -pow( 2, -10 * nowTime/duration ) + 1 ) + initNum;
}

double_t easing_expo_in_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    nowTime /= duration/2;
    if (nowTime < 1) { return changeNum/2 * pow( 2, 10 * (nowTime - 1) ) + initNum; }
    nowTime--;
    return changeNum/2 * ( -pow( 2, -10 * nowTime) + 2 ) + initNum;
}

double_t easing_expo_out_in(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    if (nowTime < duration / 2) { return easing_expo_out(nowTime*2, initNum, changeNum/2, duration  ); }
    return easing_expo(nowTime*2 - duration, initNum+changeNum/2, changeNum/2, duration );
}

double_t easing_circ(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    nowTime /= duration;
    return -changeNum * (sqrt(1 - nowTime*nowTime) - 1) + initNum;
}

double_t easing_circ_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    nowTime /= duration;
    nowTime--;
    return changeNum * sqrt(1 - nowTime*nowTime) + initNum;
}

double_t easing_circ_in_out(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    nowTime /= duration / 2;
    if (nowTime < 1) return -changeNum/2 * (sqrt(1 - nowTime*nowTime) - 1) + initNum;
    nowTime -= 2;
    return changeNum/2 * (sqrt(1 - nowTime*nowTime) + 1) + initNum;
}

double_t easing_circ_out_in(double_t nowTime, double_t initNum, double_t changeNum, double_t duration) {
    if (nowTime < duration / 2) { return easing_circ_out(nowTime*2, initNum, changeNum/2, duration  ); }
    return easing_circ(nowTime*2 - duration, initNum+changeNum/2, changeNum/2, duration );
}

static VALUE tween_alloc(VALUE self) {
    TweenData *tweenData = ALLOC(TweenData);
    return Data_Wrap_Struct(self, 0, -1, tweenData);
}

static VALUE params_hash_each(VALUE val, VALUE tween, int argc, VALUE argv[]) {
    volatile VALUE dst_hash = rb_hash_new();
    VALUE key = rb_ary_entry(argv[0], 0);
    rb_hash_aset(rb_ivar_get(tween, rb_intern("@param")), key, dst_hash);
    rb_hash_aset(rb_ivar_get(tween, rb_intern("@event_dispatch")), key, rb_hash_new());
    volatile VALUE src_hash = rb_ary_entry(argv[0], 1);
    volatile VALUE init_num = rb_hash_lookup(src_hash, ID2SYM(rb_intern("init_num")) );
    volatile VALUE goal_num = rb_hash_lookup(src_hash, ID2SYM(rb_intern("goal_num")) );
    rb_hash_aset(dst_hash, ID2SYM(rb_intern("init_num")), init_num);
    rb_hash_aset(dst_hash, ID2SYM(rb_intern("change_num")), rb_funcall(goal_num, rb_intern("-"), 1, init_num));
    rb_hash_aset(dst_hash, ID2SYM(rb_intern("current_num")), rb_float_new(0.0));
    return Qnil;
}

static VALUE tween_initialize(int argc, VALUE argv[], VALUE self) {
    TweenData* tweenData; Data_Get_Struct(self, TweenData, tweenData);
    VALUE hash, duration, easing;
    int scan_argc = rb_scan_args(argc, argv, "12", &hash, &duration, &easing);
    rb_ivar_set(self, rb_intern("@param"), rb_hash_new());
    rb_ivar_set(self, rb_intern("@event_dispatch"), rb_hash_new());
    rb_block_call(hash, rb_intern("each"), 0, NULL, params_hash_each, self);
    switch (scan_argc) {
        case 1:
            duration = rb_float_new(1.0);
            easing = ID2SYM(rb_intern("none"));
            break;
        case 2:
            easing = ID2SYM(rb_intern("none"));
            break;        
    }
    tweenData->duration = NUM2INT(duration);
    tweenData->nowTime = 0; 
    VALUE easing_str = rb_sym_to_s(easing);

    const char* result= StringValuePtr(easing_str);
    size_t result_length = strlen(result);
    if (strncmp(result, "none", result_length) == 0) {
        tweenData->easing = easing_none;
    } else if (strncmp(result, "quad", result_length) == 0) {
        tweenData->easing = easing_in_quad;
    } else if (strncmp(result, "quad_out", result_length) == 0) {
        tweenData->easing = easing_out_quad;
    } else if (strncmp(result, "quad_inout", result_length) == 0) {
        tweenData->easing = easing_in_out_quad;       
    } else if (strncmp(result, "quad_outin", result_length) == 0) {
        tweenData->easing = easing_out_in_quad;       
    } else if (strncmp(result, "cubic", result_length) == 0) {
        tweenData->easing = easing_cubic;       
    } else if (strncmp(result, "cubic_out", result_length) == 0) {
        tweenData->easing = easing_cubic_out;       
    } else if (strncmp(result, "cubic_inout", result_length) == 0) {
        tweenData->easing = easing_cubic_in_out;       
    } else if (strncmp(result, "cubic_outin", result_length) == 0) {
        tweenData->easing = easing_cubic_out_in;       
    } else if (strncmp(result, "quart", result_length) == 0) {
        tweenData->easing = easing_quart;
    } else if (strncmp(result, "quart_out", result_length) == 0) {
        tweenData->easing = easing_quart_out;
    } else if (strncmp(result, "quart_inout", result_length) == 0) {
        tweenData->easing = easing_quart_in_out;
    } else if (strncmp(result, "quart_outin", result_length) == 0) {
        tweenData->easing = easing_quart_out_in;
    } else if (strncmp(result, "quint", result_length) == 0) {
        tweenData->easing = easing_quint;
    } else if (strncmp(result, "quint_out", result_length) == 0) {
        tweenData->easing = easing_quint_out;
    } else if (strncmp(result, "quint_in_out", result_length) == 0) {
        tweenData->easing = easing_quint_in_out;
    } else if (strncmp(result, "quint_out_in", result_length) == 0) {
        tweenData->easing = easing_quint_out_in;
    } else if (strncmp(result, "sin", result_length) == 0) {
        tweenData->easing = easing_sin;
    } else if (strncmp(result, "sin_out", result_length) == 0) {
        tweenData->easing = easing_sin_out;
    } else if (strncmp(result, "sin_in_out", result_length) == 0) {
        tweenData->easing = easing_sin_in_out;
    } else if (strncmp(result, "sin_out_in", result_length) == 0) {
        tweenData->easing = easing_sin_out_in;
    } else if (strncmp(result, "expo", result_length) == 0) {
        tweenData->easing = easing_expo;
    } else if (strncmp(result, "expo_out", result_length) == 0) {
        tweenData->easing = easing_expo_out;
    } else if (strncmp(result, "expo_in_out", result_length) == 0) {
        tweenData->easing = easing_expo_in_out;
    } else if (strncmp(result, "expo_out_in", result_length) == 0) {
        tweenData->easing = easing_expo_out_in;
    } else if (strncmp(result, "circ", result_length) == 0) {
        tweenData->easing = easing_circ;
    } else if (strncmp(result, "circ_out", result_length) == 0) {
        tweenData->easing = easing_circ_out;
    } else if (strncmp(result, "circ_in_out", result_length) == 0) {
        tweenData->easing = easing_circ_in_out;
    } else if (strncmp(result, "circ_out_in", result_length) == 0) {
        tweenData->easing = easing_circ_out_in;

    }
    VALUE event_manager = rb_class_new_instance(0, NULL, rb_path2class("SDLUdon::Event::Manager"));
    rb_ivar_set(self, rb_intern("@event_manager"), event_manager);
    tweenData->active = Qtrue;
    return Qnil;
}

static VALUE tween_update_complete_param_each(VALUE val, VALUE self, int argc, VALUE argv[]) {
    VALUE v = rb_ary_entry(argv[0], 1);
    volatile VALUE current_num = rb_funcall(rb_hash_lookup(v, ID2SYM(rb_intern("init_num")) ) , rb_intern("+"), 1, rb_hash_lookup(v,  ID2SYM(rb_intern("change_num"))) );
    rb_hash_aset(v, ID2SYM( rb_intern("current_num")), current_num );
    volatile VALUE dispatch_value_hash = rb_hash_lookup(rb_ivar_get(self, rb_intern("@event_dispatch")), rb_ary_entry(argv[0], 0) );
    rb_hash_aset(dispatch_value_hash, ID2SYM( rb_intern("current_num")), current_num);
    rb_hash_aset(dispatch_value_hash, ID2SYM( rb_intern("rate")), rb_float_new(1.0));
    return Qnil;
}

static VALUE tween_update_enterframe_param_each(VALUE val, VALUE self, int argc, VALUE argv[]) {
    TweenData* tweenData; Data_Get_Struct(self, TweenData, tweenData);
    volatile VALUE v = rb_ary_entry(argv[0], 1);
    double_t nowTime = tweenData->nowTime;
    double_t initNum = NUM2DBL(rb_hash_lookup(v, ID2SYM(rb_intern("init_num") ) ));
    double_t changeNum = NUM2DBL(rb_hash_lookup(v, ID2SYM(rb_intern("change_num") ) ));
    double_t duration = tweenData->duration;
    volatile VALUE current_num = rb_float_new( tweenData->easing(nowTime, initNum, changeNum, duration));
    rb_hash_aset(v, ID2SYM(rb_intern("current_num")), current_num);
    volatile VALUE dispatch_value_hash = rb_hash_lookup(rb_ivar_get(self, rb_intern("@event_dispatch")), rb_ary_entry(argv[0], 0) );
    rb_hash_aset(dispatch_value_hash, ID2SYM( rb_intern("current_num")), current_num);
    rb_hash_aset(dispatch_value_hash, ID2SYM( rb_intern("rate")), rb_float_new(nowTime / duration));
    return Qnil;        
}

static VALUE tween_update(int argc, int argv[], VALUE self) {
    TweenData* tweenData; Data_Get_Struct(self, TweenData, tweenData);
    if (tweenData->active == Qfalse) return Qnil;
    tweenData->nowTime++;
    VALUE event_manager = rb_ivar_get(self, rb_intern("@event_manager"));
    VALUE param = rb_ivar_get(self, rb_intern("@param"));
    if (tweenData->nowTime >= tweenData->duration) {
        rb_block_call(param, id_each, 0, NULL, tween_update_complete_param_each, self);
        rb_funcall(event_manager, id_trigger, 2, ID2SYM(rb_intern("complete")), rb_ivar_get(self, rb_intern("@event_dispatch")));
        tweenData->active = Qfalse;
    } else {
        rb_block_call(param, rb_intern("each"), 0, NULL, tween_update_enterframe_param_each, self);
        rb_funcall(event_manager, rb_intern("trigger"), 2, ID2SYM(rb_intern("enterframe")), rb_ivar_get(self, rb_intern("@event_dispatch")));
    }
    return Qnil;
}

static VALUE tween_get_active(VALUE self) {
    TweenData* tweenData; Data_Get_Struct(self, TweenData, tweenData);
    return tweenData->active;
}

static VALUE tween_get_event_now(VALUE self) {
    TweenData* tweenData; Data_Get_Struct(self, TweenData, tweenData);
    return tweenData->nowTime;
}


static VALUE tween_get_event_manager(VALUE self) {
    return rb_ivar_get(self, rb_intern("@event_manager"));
}


void Init_tween(VALUE parent) {
    VALUE tween_class = rb_define_class_under(parent, "Tween", rb_cObject);
    rb_define_alloc_func(tween_class, tween_alloc);
    rb_define_private_method(tween_class, "initialize", tween_initialize, -1);
    rb_define_method(tween_class, "update", tween_update, -1);
    rb_define_alias(tween_class, "call", "update");
    rb_define_method(tween_class, "active?", tween_get_active, 0);
    rb_define_method(tween_class, "event", tween_get_event_manager, 0);
    rb_define_method(tween_class, "now", tween_get_event_now, 0);
    rb_define_method(tween_class, "current", tween_get_event_now, 0);
}
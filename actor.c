#include "sdl_udon_private.h"

static volatile VALUE sym_enterframe = Qundef;
static volatile VALUE sym_deactive = Qundef;

static VALUE actor_decide_mouse_event(VALUE self) {
    VALUE class_input = rb_path2class("SDLUdon::Input");
    int mx = NUM2INT(rb_funcall(class_input, rb_intern("mouse_x"), 0));
    int my = NUM2INT(rb_funcall(class_input, rb_intern("mouse_y"), 0));
    int x = NUM2INT(rb_funcall(self, rb_intern("x"), 0));    
    int y = NUM2INT(rb_funcall(self, rb_intern("y"), 0));    
    int x2 = NUM2INT(rb_funcall(self, rb_intern("x2"), 0));
    int y2 = NUM2INT(rb_funcall(self, rb_intern("y2"), 0));
    if (x > mx || mx > x2 || y > my || my > y2) {
        if (rb_ivar_get(self, rb_intern("@mouse_hover")) == Qtrue) {
            rb_ivar_set(self, rb_intern("@mouse_hover"), Qfalse);
            VALUE event = rb_ivar_get(self, id_iv_event_manager);
            rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("mouse_hover_out")), Qnil);
        }
        return Qnil;
    }
    VALUE event = rb_ivar_get(self, id_iv_event_manager);
    rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("mouse_hover")), Qnil);
    rb_ivar_set(self, rb_intern("@mouse_hover"), Qtrue);
    volatile VALUE push_ary = rb_funcall(class_input, rb_intern("push_mouses"), 0);
    volatile VALUE down_ary = rb_funcall(class_input, rb_intern("down_mouses"), 0);
    volatile VALUE up_ary = rb_funcall(class_input, rb_intern("up_mouses"), 0);

    if (RARRAY_LENINT(push_ary) != 0) {
        rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("mouse_push")), push_ary);
        if (rb_ary_includes(push_ary, ID2SYM(rb_intern("lbtn"))) == Qtrue) {
            rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("click")), push_ary);
        }
    }
    if (RARRAY_LENINT(down_ary) != 0)
        rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("mouse_down")), down_ary);
    if (RARRAY_LENINT(up_ary) != 0)
        rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("mouse_up")), up_ary);

    if (RARRAY_LENINT(push_ary) != 0 || RARRAY_LENINT(down_ary) != 0 ||
        RARRAY_LENINT(up_ary) != 0) 
    {
        volatile VALUE key_event_data = rb_hash_new();
        rb_hash_aset(key_event_data, ID2SYM(rb_intern("push")), push_ary);
        rb_hash_aset(key_event_data, ID2SYM(rb_intern("up")), up_ary);
        rb_hash_aset(key_event_data, ID2SYM(rb_intern("down")), down_ary);
        rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("mouse")), key_event_data);
    }
    return self;
}

static VALUE actor_decide_key_event(VALUE self) {
    VALUE class_input = rb_path2class("SDLUdon::Input");
    volatile VALUE push_ary = rb_funcall(class_input, rb_intern("push_keys"), 0);
    volatile VALUE down_ary = rb_funcall(class_input, rb_intern("down_keys"), 0);
    volatile VALUE up_ary = rb_funcall(class_input, rb_intern("up_keys"), 0);
    VALUE event = rb_ivar_get(self, id_iv_event_manager);
    if (RARRAY_LENINT(push_ary) != 0)
        rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("key_push")), push_ary);
    if (RARRAY_LENINT(down_ary) != 0)
        rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("key_down")), down_ary);
    if (RARRAY_LENINT(up_ary) != 0)
        rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("key_up")), up_ary);
    
    if (RARRAY_LENINT(push_ary) != 0 || RARRAY_LENINT(down_ary) != 0 ||
        RARRAY_LENINT(up_ary) != 0) 
    {
        volatile VALUE key_event_data = rb_hash_new();
        rb_hash_aset(key_event_data, ID2SYM(rb_intern("push")), push_ary);
        rb_hash_aset(key_event_data, ID2SYM(rb_intern("up")), up_ary);
        rb_hash_aset(key_event_data, ID2SYM(rb_intern("down")), down_ary);
        rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("key")), key_event_data);
    }
    return self;
}

static VALUE actor_update_event(VALUE self) {
    rb_funcall(self, rb_intern("decide_key_event"), 0);
    rb_funcall(self, rb_intern("decide_mouse_event"), 0);
    return Qnil;
}

static void actor_mark(ActorData *actor) {
    rb_gc_mark(actor->x);
    rb_gc_mark(actor->y);
}

static VALUE actor_alloc(VALUE klass) {
    ActorData *actor = ALLOC(ActorData);
    actor->x = rb_int_new(0); actor->y = rb_int_new(0);
    return Data_Wrap_Struct(klass, actor_mark, -1, actor);
}


static VALUE actor_get_width(VALUE self) {
    VALUE image = rb_ivar_get(self, rb_intern("@image"));
    if (NIL_P(image)) return rb_int_new(0);
    ImageData *imageData; Data_Get_Struct(image, ImageData, imageData);
    return rb_int_new(imageData->rect->w);
}

static VALUE actor_get_height(VALUE self) {
    VALUE image = rb_ivar_get(self, rb_intern("@image"));
    if (NIL_P(image)) return rb_int_new(0);
    ImageData *imageData; Data_Get_Struct(image, ImageData, imageData);
    return rb_int_new(imageData->rect->h);
}

static int ParseActorOption(VALUE key, VALUE value, VALUE self) {
    volatile VALUE keystr = rb_funcall(key, rb_intern("to_s"), 0);
    const char* str = StringValuePtr(keystr); int len = strlen(str);
    if (strncmp(str, "move", len) == 0) {
        rb_funcall(self, rb_intern("move"), 2, RARRAY_PTR(value)[0],RARRAY_PTR(value)[1]);
    } else if (strncmp(str, "event", len) == 0) {
        VALUE event_manager = rb_ivar_get(self, id_iv_event_manager);
        VALUE value_ary = rb_funcall(value, rb_intern("to_a"), 0);
        int i, len = RARRAY_LENINT(value_ary);
        for (i = 0; i < len; ++i) {
            volatile VALUE pair = RARRAY_PTR(value_ary)[i];
            rb_funcall_with_block(event_manager, rb_intern("add"), 1, (VALUE[]){RARRAY_PTR(pair)[0]}, RARRAY_PTR(pair)[1]);
        }
    }
    return ST_CONTINUE;
}

static VALUE actor_singleton_swap_image(VALUE self, VALUE index) {
    VALUE images = rb_ivar_get(self, rb_intern("@images"));
    int cIndex = NUM2INT(index);
    int imageSize = RARRAY_LENINT(images);
    if (cIndex < 0 || cIndex >= imageSize) return Qnil;
    return rb_ivar_set(self, rb_intern("@image"), rb_ary_entry(images, NUM2INT(index)));
}

static VALUE actor_initialize(int argc, VALUE argv[], VALUE self) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    actor->visible = TRUE;
    actor->suspend = FALSE;
    volatile VALUE opt;
    if (rb_scan_args(argc, argv, "01", &opt) == 0) { opt = rb_hash_new(); }
    VALUE image = rb_hash_lookup(opt, ID2SYM(rb_intern("image")));
    rb_ivar_set(self, rb_intern("@mouse_hover"), Qfalse);
    if(rb_ivar_defined(self, rb_intern("@image")) == Qfalse) {
        rb_ivar_set(self, rb_intern("@image"), Qnil);
        if (!NIL_P(image)) {
            if (rb_type_p(image, T_ARRAY)) {
                rb_define_singleton_method(self, "swap_image", actor_singleton_swap_image, 1);
                rb_ivar_set(self, rb_intern("@images"), image);
                rb_ivar_set(self, rb_intern("@image"), RARRAY_PTR(image)[0]);
            } else {
                rb_ivar_set(self, rb_intern("@image"), image);
            }
        }
    }
    actor->active = actor->collidable = TRUE;
    rb_ivar_set(self, rb_intern("@event_manager"), rb_class_new_instance(0, NULL, rb_path2class("SDLUdon::Event::Manager")));
    rb_ivar_set(self, rb_intern("@timeline"), rb_class_new_instance(1, (VALUE[]) { self }, rb_path2class("SDLUdon::Timeline")));
    rb_ivar_set(self, id_iv_state_machine, rb_class_new_instance(0, NULL, rb_path2class("SDLUdon::StateMachine")));
    if (rb_respond_to(self, rb_intern("init_collision"))) {
        volatile VALUE width = rb_funcall(self, rb_intern("width"), 0);
        if (NUM2INT(width) == 0) {
            rb_funcall(self, rb_intern("init_collision"), 0); 
        } else {
            VALUE ary = rb_ary_new3(4, rb_int_new(0), rb_int_new(0), width, rb_funcall(self, rb_intern("height"), 0));
            rb_funcall(self, rb_intern("init_collision"), 1, ary); 
        }
    }
    rb_hash_foreach(opt, ParseActorOption, self);
    return Qnil;
}

static VALUE actor_load(int argc, VALUE argv[], VALUE self) {
    volatile VALUE filename, opt;
    if (rb_scan_args(argc, argv, "11", &filename, &opt) == 1) { opt = rb_hash_new(); }
    volatile VALUE window_id = rb_hash_lookup(opt, ID2SYM(rb_intern("window_id")));
    VALUE class_image = rb_path2class("SDLUdon::Image");
    volatile VALUE image = rb_funcall(class_image, rb_intern("load"), 2, filename, (NIL_P(window_id)) ? rb_int_new(0) : window_id);
    rb_hash_aset(opt, ID2SYM(rb_intern("image")), image);
    volatile VALUE actor = rb_class_new_instance(1, (VALUE []){ opt }, self);
    return actor;
}


static VALUE actor_get_x(VALUE self) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    return actor->x;
}

static VALUE actor_get_x2(VALUE self) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    VALUE image = rb_ivar_get(self, rb_intern("@image"));
    int width;
    if (NIL_P(image)) {
        width = 0;
    } else {
        ImageData* imageData;Data_Get_Struct(image, ImageData, imageData);
        width = imageData->rect->w;
    } 
    return rb_int_new(NUM2INT(actor->x) + width);
}

static VALUE actor_set_x(VALUE self, VALUE x) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    actor->x = x; 
    return self;
}

static VALUE actor_get_y(VALUE self) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    return actor->y;
}

static VALUE actor_get_y2(VALUE self) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    VALUE image = rb_ivar_get(self, rb_intern("@image"));
    int height;
    if (NIL_P(image)) {
        height = 0;
    } else {
        ImageData* imageData;Data_Get_Struct(image, ImageData, imageData);
        height = imageData->rect->h;
    } 
    return rb_int_new(NUM2INT(actor->y) + height);
}


static VALUE actor_set_y(VALUE self, VALUE y) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    actor->y = y;
    return self;
}

static VALUE actor_move(VALUE self, VALUE x, VALUE y) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    actor->x = x; actor->y = y;
    return self;
}

static VALUE actor_move_add(int argc, VALUE argv[], VALUE self) {
    VALUE args;
    rb_scan_args(argc, argv, "*", &args);
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    if (RARRAY_LENINT(args) == 0) return Qnil;
    if (RARRAY_LENINT(args) == 1) {
        VALUE enumlator = RARRAY_PTR(args)[0];
        if (RB_TYPE_P(enumlator, T_HASH)) {
            VALUE x = rb_hash_lookup(enumlator, ID2SYM(rb_intern("x")));
            VALUE y = rb_hash_lookup(enumlator, ID2SYM(rb_intern("y")));
            actor->x = rb_funcall(x, rb_intern("to_i"), 0);
            actor->y = rb_funcall(y, rb_intern("to_i"), 0);
        } else {
            actor->x = rb_funcall(rb_ary_entry(enumlator, 0), rb_intern("to_i"), 0);
            actor->y = rb_funcall(rb_ary_entry(enumlator, 1), rb_intern("to_i"), 0);
        }
    } else {
        actor->x = rb_funcall(rb_ary_entry(args, 0), rb_intern("to_i"), 0);
        actor->y = rb_funcall(rb_ary_entry(args, 1), rb_intern("to_i"), 0);
    }
    return self;
}


static VALUE actor_update(VALUE self) {
    ActorData* actorData; Data_Get_Struct(self, ActorData, actorData);
    if (actorData->suspend) return Qnil;
    rb_funcall(self, rb_intern("update_event"), 0);
    rb_funcall(rb_ivar_get(self, id_iv_state_machine), id_run, 0);
    rb_funcall(rb_ivar_get(self, id_iv_timeline), id_update, 0);
    return self;
}

static VALUE actor_get_timeline(VALUE self) { return rb_ivar_get(self, id_iv_timeline); }

static VALUE actor_event_manager(int argc, VALUE argv[], VALUE self) {
    VALUE opt_hash;
    rb_scan_args(argc, argv, "01", &opt_hash);
    if (NIL_P(opt_hash)) {
        return rb_ivar_get(self, id_iv_event_manager);
    } else {
        VALUE event = rb_ivar_get(self, id_iv_event_manager);
        volatile VALUE opt_ary = rb_funcall(opt_hash, rb_intern("to_a"), 0);
        int i, len = RARRAY_LENINT(opt_ary);
        for (i = 0; i < len; ++i) {
            volatile VALUE item = RARRAY_PTR(opt_ary)[i];
            rb_funcall_with_block(event, rb_intern("add"), 1, (VALUE[]){RARRAY_PTR(item)[0]}, RARRAY_PTR(item)[1]);
        }
        return self;
    }
}

static VALUE actor_hide(VALUE self) {
    ActorData* actorData;Data_Get_Struct(self, ActorData, actorData);
    actorData->visible = FALSE;
    return self;
}

static VALUE actor_show(VALUE self) {
    ActorData* actorData;Data_Get_Struct(self, ActorData, actorData);
    actorData->visible = TRUE;
    return self;
}


static VALUE actor_get_active(VALUE self) {
    ActorData* actorData; Data_Get_Struct(self, ActorData, actorData);
    return actorData->active ? Qtrue : Qfalse;
}

static VALUE actor_get_halt(VALUE self) {
    ActorData* actorData; Data_Get_Struct(self, ActorData, actorData);
    return actorData->active ? Qfalse : Qtrue;
}

static VALUE actor_deactive(VALUE self) {
    ActorData* actorData; Data_Get_Struct(self, ActorData, actorData);
    actorData->active = FALSE;
    rb_funcall(rb_ivar_get(self, id_iv_event_manager), id_trigger, 2, sym_deactive, Qnil);
    return Qnil;
}

static inline double CalcDistance(VALUE a, VALUE b) {
    ActorData* aData; Data_Get_Struct(a, ActorData, aData);
    ActorData* bData; Data_Get_Struct(a, ActorData, bData);
    double aX = NUM2DBL(aData->x), aY = NUM2DBL(aData->y); 
    double bX = NUM2DBL(bData->x), bY = NUM2DBL(bData->y); 
    double x = (bX - aX) * (bX - aX), y = (bY - aY) * (bY - aY);
    return x + y;
}

static VALUE actor_distance(VALUE self, VALUE target) { return rb_float_new(sqrt(CalcDistance(self, target))); }
static VALUE actor_distance_square(VALUE self, VALUE target) { return rb_float_new(CalcDistance(self, target)); }

static VALUE actor_hit_area(int argc, VALUE argv[], VALUE self) {
    VALUE hit_area = rb_call_super(0, NULL);
    VALUE area; rb_scan_args(argc, argv, "*", &area);
    if ( RARRAY_LENINT(area) == 0) return hit_area;
    if (RB_TYPE_P(RARRAY_PTR(area)[0], T_ARRAY)) {
        rb_funcall(hit_area, rb_intern("set"), 1, RARRAY_PTR(area)[0]);
    } else {
        rb_funcall(hit_area, rb_intern("set"), 1, area);
    }
    return self;
}

static VALUE actor_to_a(VALUE self) {
    return rb_funcall(self, rb_intern("real_hit_area"), 0); 
}

static VALUE actor_hit(int argc, VALUE argv[], VALUE self) {
    VALUE target, event_able;
    if (rb_scan_args(argc, argv, "11", &target, &event_able) == 1) { event_able = Qtrue; }
    volatile VALUE result = Qundef;
    VALUE scene_class = rb_path2class("SDLUdon::Scene");
    BOOL target_is_scene = (rb_obj_is_kind_of(target, scene_class) == Qtrue);
    if (target_is_scene) {
        result = rb_hash_new();
        int i; BOOL hitFlag = FALSE;
        VALUE target_pool = rb_ivar_get(target, rb_intern("@pool"));
        int targetLen = RARRAY_LENINT(target_pool);
        for (i = 0; i < targetLen; ++i) {
            VALUE target_child = RARRAY_PTR(target_pool)[i];
            volatile VALUE hit_data = rb_call_super(1, (VALUE[]){ target_child });
            if (!NIL_P(hit_data)) {
                hitFlag  = TRUE; rb_hash_aset(result, target_child, hit_data);
            }
        }
        if (!hitFlag) { return Qnil; }
    } else {
        result = rb_call_super(1, argv);
        if (NIL_P(result)) { return Qnil; }
    }
    VALUE event = rb_ivar_get(self, rb_intern("@event_manager"));
    if (event_able == Qtrue) { rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("hit")), result); }
    if (rb_block_given_p()) { rb_yield(result); return self; }
    return result;
}

static VALUE actor_set_collied_active(VALUE self, VALUE true_or_false) {
    ActorData* actorData;Data_Get_Struct(self, ActorData, actorData);
    actorData->collidable = true_or_false == Qtrue ? TRUE : FALSE;
    return Qnil;
}

static VALUE actor_get_collied_active(VALUE self) {
    ActorData* actorData;Data_Get_Struct(self, ActorData, actorData);
    return actorData->collidable ? Qtrue : Qfalse;
}

static VALUE actor_get_state_machine(int argc, VALUE argv[], VALUE self) {
    VALUE hash; rb_scan_args(argc, argv, "01", &hash);
    if (NIL_P(hash)) {
        return rb_ivar_get(self, rb_intern("@state_machine"));
    } else {
        if (rb_block_given_p()) {
            return rb_funcall(rb_ivar_get(self, rb_intern("@state_machine")), rb_intern("add"), 2, ID2SYM(rb_intern("main")), rb_block_proc());
        } else {
            return rb_funcall(rb_ivar_get(self, rb_intern("@state_machine")), rb_intern("add"), 1, hash);
        }
    }
} 

static VALUE actor_method_missing(int argc, VALUE argv[], VALUE self) {
    if (!rb_respond_to(self, SYM2ID(argv[0]))) {
        if (NIL_P(argv[2]) && rb_proc_lambda_p(argv[1])) {
            rb_funcall(self, rb_intern("define_singleton_method"), 2, argv[0], argv[1]);
            return self;
        }
    }
    return rb_call_super(argc, argv);
}

static VALUE actor_is_visible(VALUE self) {
    ActorData* actor; Data_Get_Struct(self, ActorData, actor);
    return actor->visible ? Qtrue : Qfalse;
}

static VALUE actor_pause(VALUE self) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    actor->suspend = TRUE;
    rb_funcall(self, rb_intern("hide"), 0);
    return self;
}

static VALUE actor_is_pause(VALUE self) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    return actor->suspend == TRUE ? Qtrue : Qfalse;
}

static VALUE actor_resume(VALUE self) {
    ActorData *actor; Data_Get_Struct(self, ActorData, actor);
    actor->suspend = FALSE;
    rb_funcall(self, rb_intern("show"), 0);
    return self;
}

void Init_actor(VALUE parent_module) {
    VALUE actor = rb_define_class_under(parent_module, "Actor", rb_cObject);
    rb_include_module(actor, rb_path2class("SDLUdon::Collision"));
    rb_define_alloc_func(actor, actor_alloc);
    rb_define_singleton_method(actor, "load", actor_load, -1);
    rb_define_private_method(actor, "initialize", actor_initialize, -1);
    rb_define_private_method(actor, "halt?", actor_get_halt, 0);
    rb_define_private_method(actor, "update_event", actor_update_event, 0);
    rb_define_private_method(actor, "decide_key_event", actor_decide_key_event, 0);
    rb_define_private_method(actor, "decide_mouse_event", actor_decide_mouse_event, 0);
    rb_define_private_method(actor, "method_missing", actor_method_missing, -1);
    rb_define_method(actor, "deactive", actor_deactive, 0);
    rb_define_method(actor, "active?", actor_get_active, 0);
    rb_define_method(actor, "collidable=", actor_set_collied_active, 1);
    rb_define_method(actor, "collidable?", actor_get_collied_active, 0);
    rb_define_method(actor, "update", actor_update, 0);
    rb_define_method(actor, "x", actor_get_x, 0);
    rb_define_method(actor, "x2", actor_get_x2, 0);
    rb_define_method(actor, "x=", actor_set_x, 1);
    rb_define_method(actor, "y", actor_get_y, 0);
    rb_define_method(actor, "y2", actor_get_y2, 0);
    rb_define_method(actor, "y=", actor_set_y, 1);
    rb_define_method(actor, "width", actor_get_width, 0);
    rb_define_method(actor, "height", actor_get_height, 0);
    rb_define_method(actor, "move", actor_move, 2);
    rb_define_method(actor, "move_add", actor_move_add, -1);
    rb_define_method(actor, "tl", actor_get_timeline, 0);
    rb_define_method(actor, "event", actor_event_manager, -1);
    rb_define_method(actor, "hit", actor_hit, -1); // ユーザー任意
    rb_define_method(actor, "hide", actor_hide, 0);
    rb_define_method(actor, "show", actor_show, 0);
    rb_define_method(actor, "visible?", actor_is_visible, 0);
    rb_define_method(actor, "distance", actor_distance, 1);
    rb_define_method(actor, "distance_square", actor_distance_square, 1);
    rb_define_method(actor, "hit_area", actor_hit_area, -1); // module Collisionに定義すべきか？
    rb_define_method(actor, "to_a", actor_to_a, 0);
    rb_define_method(actor, "state", actor_get_state_machine, -1);
    rb_define_method(actor, "pause", actor_pause, 0);
    rb_define_method(actor, "pause?", actor_is_pause, 0);
    rb_define_method(actor, "resume", actor_resume, 0);
    rb_define_attr(actor, "image", 1, 1);

    sym_enterframe = ID2SYM(rb_intern("enterframe"));
    sym_deactive = ID2SYM(rb_intern("deactive"));
}
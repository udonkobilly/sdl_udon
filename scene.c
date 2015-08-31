#include "sdl_udon_private.h"

static volatile VALUE sym_enterframe = Qundef;

static VALUE scene_initialize(int argc, VALUE argv[], VALUE self) {
    rb_call_super(argc, argv);
    rb_ivar_set(self, rb_intern("@child_lock"), Qfalse);
    volatile VALUE opt;
    if (rb_scan_args(argc, argv, "01", &opt) == 0) { opt = rb_hash_new(); }
    volatile VALUE screen = rb_hash_lookup(opt, ID2SYM(rb_intern("screen")));
    if (NIL_P(screen) ) {
        screen = rb_obj_alloc(rb_path2class("SDLUdon::Screen"));
        int width, height; SDL_GetWindowSize(systemData->window, &width, &height);
        rb_obj_call_init(screen, 2, (VALUE []) { rb_int_new(width), rb_int_new(height) } );
        rb_ivar_set(self, rb_intern("@screen"), screen);
    } else {
        rb_ivar_set(self, rb_intern("@screen"), screen);
    }
    rb_ivar_set(self, rb_intern("@pool"), rb_ary_new()); // @_child_pool_くらいにしておかないと危ない
    return Qnil;
}

static VALUE scene_draw(int argc, VALUE argv[], VALUE self) {
    VALUE render_target;
    if (rb_scan_args(argc, argv, "01", &render_target) == 0) render_target =  rb_ivar_get(self, id_iv_screen);
    VALUE ary = rb_ivar_get(self, id_iv_pool);
    rb_ivar_set(self, rb_intern("@child_lock"), Qtrue);   
    int size = RARRAY_LENINT(ary);
    VALUE *arys = RARRAY_PTR(ary);
    VALUE child = Qundef;
    volatile VALUE x, y, opts, image, visible;
    image = rb_ivar_get(self, id_iv_image);
    if (!NIL_P(image)) {
        x = rb_funcall(self, id_x, 0);     
        y = rb_funcall(self, id_y, 0);
        opts = rb_respond_to(self, id_draw_opts) ? rb_funcall(self, id_draw_opts, 0) : Qnil;
        visible = rb_respond_to(self, id_is_visible) ? rb_funcall(self, id_is_visible, 0) : Qnil;
        if (NIL_P(visible) || (visible == Qtrue))
            rb_funcall(render_target, id_draw_ex, 4, image, x,y,opts);
    }
    for ( int i = 0; i < size; ++i) {
        child = arys[i];
        if (rb_respond_to(child, id_draw)) {
            rb_funcall(child, id_draw, 1, render_target);
        } else {
            image = rb_ivar_get(child, id_iv_image);
            if (NIL_P(image)) continue;
            x = rb_funcall(child, id_x, 0);     
            y = rb_funcall(child, id_y, 0);
            opts = rb_respond_to(child, id_draw_opts) ? rb_funcall(child, id_draw_opts, 0) : Qnil;
            visible = rb_respond_to(child, id_is_visible) ? rb_funcall(child, id_is_visible, 0) : Qnil;
            if (NIL_P(visible) || (visible == Qtrue))
                rb_funcall(render_target, id_draw_ex, 4, image, x, y, opts);
        }
    }
    rb_ivar_set(self, rb_intern("@child_lock"), Qfalse);
    return self;
}

static VALUE scene_update_child( VALUE child, VALUE nil, int argc, VALUE argv[]) {
    rb_funcall(child, rb_intern("update"), 0);
    return rb_funcall(child, rb_intern("active?"), 0) == Qtrue ? Qfalse : Qtrue;
}

static VALUE scene_update(VALUE self) {
    ActorData* sceneData; Data_Get_Struct(self, ActorData, sceneData);
    if (sceneData->suspend) return Qnil;
    if (sceneData->collidable) { rb_funcall(self, rb_intern("each_hit"), 0); }
    rb_funcall(self, rb_intern("update_event"), 0);
    rb_funcall(rb_ivar_get(self, id_iv_state_machine), id_run, 0);
    volatile VALUE ary = rb_ivar_get(self, id_iv_pool);
    rb_block_call(ary, id_delete_if, 0, NULL, scene_update_child, Qnil);
    return self;
}

static VALUE scene_each_child_body( VALUE val, VALUE block_proc, int argc, VALUE argv[]) {
    rb_proc_call_with_block(block_proc, 1, (VALUE[]) { val }, Qnil);
    return Qnil;
}

static VALUE scene_each_child(VALUE self) {
    if (!rb_block_given_p()) return rb_ary_each(rb_ivar_get(self, id_iv_pool));
    volatile VALUE ary = rb_ivar_get(self, id_iv_pool);
    rb_block_call(ary, id_each, 0, NULL, scene_each_child_body, rb_block_proc());
    return self;
}

VALUE scene_clear_child(VALUE self, VALUE actor) {
    if (rb_ivar_get(self, rb_intern("@child_lock"))) return Qnil;
    VALUE ary = rb_ivar_get(self, id_iv_pool);
    rb_ary_clear(ary);
    return self;
}
static VALUE scene_sort_child(VALUE self) {
    rb_need_block();
    if (rb_ivar_get(self, rb_intern("@child_lock"))) return Qnil;
    VALUE ary = rb_ivar_get(self, id_iv_pool);
    rb_funcall_with_block(ary, rb_intern("sort"), 0, NULL, rb_block_proc());
    return self;
}

static VALUE scene_unshift_child(int argc, VALUE argv[], VALUE self) {
    if (rb_ivar_get(self, rb_intern("@child_lock"))) return Qnil;
    VALUE ary; rb_scan_args(argc, argv, "*", &ary);
    volatile VALUE flatten_ary;
    flatten_ary = RB_TYPE_P(RARRAY_PTR(ary)[0], T_ARRAY) ? rb_funcall(ary, rb_intern("flatten"), 0) : ary;

    volatile VALUE pool = rb_ivar_get(self, id_iv_pool);
    int i, len = RARRAY_LENINT(flatten_ary);
    for (i = len-1; i >= 0; --i) rb_ary_unshift(pool, RARRAY_PTR(flatten_ary)[i]);
    return self;
}

VALUE scene_add_child(int argc, VALUE argv[], VALUE self) {
    VALUE ary; rb_scan_args(argc, argv, "*", &ary);
    volatile VALUE flatten_ary;
    flatten_ary = RB_TYPE_P(RARRAY_PTR(ary)[0], T_ARRAY) ? rb_funcall(ary, rb_intern("flatten"), 0) : ary;
    volatile VALUE pool = rb_ivar_get(self, id_iv_pool);
    int i, len = RARRAY_LENINT(flatten_ary);
    for (i = 0; i < len; ++i) rb_ary_push(pool, RARRAY_PTR(flatten_ary)[i]);
    return self;
}

static VALUE scene_find_child(int argc, VALUE argv[], VALUE self) {
    if (rb_ivar_get(self, rb_intern("@child_lock"))) return Qnil;
    VALUE ary = rb_ivar_get(self, id_iv_pool);
    return rb_funcall_with_block(ary, rb_intern("find"), argc, argv, rb_block_proc());
}

static VALUE scene_find_children(int argc, VALUE argv[], VALUE self) {
    if (rb_ivar_get(self, rb_intern("@child_lock"))) return Qnil;
    VALUE ary = rb_ivar_get(self, id_iv_pool);
    return rb_funcall_with_block(ary, rb_intern("find_all"), argc, argv, rb_block_proc());
}

static VALUE scene_delete_child(int argc, VALUE argv[], VALUE self) {
    if (rb_ivar_get(self, rb_intern("@child_lock"))) return Qnil;
    VALUE childs; rb_scan_args(argc, argv, "*", &childs);
    volatile VALUE flatten_childs;
    flatten_childs = RB_TYPE_P(RARRAY_PTR(childs)[0], T_ARRAY) ? rb_funcall(childs, rb_intern("flatten"), 0) : childs;
    int i, len = RARRAY_LENINT((flatten_childs));
    VALUE ary = rb_ivar_get(self, id_iv_pool);
    for (i = 0; i < len; ++i) { rb_ary_delete(ary, RARRAY_PTR(flatten_childs)[i]); }
    return self;
}

static VALUE scene_delete_child_at(int argc, VALUE argv[], VALUE self) {
    if (rb_ivar_get(self, rb_intern("@child_lock"))) return Qnil;
    VALUE child_indexes; rb_scan_args(argc, argv, "*", &child_indexes);
    volatile VALUE flatten_child_indexes = rb_funcall(child_indexes, rb_intern("flatten"), 0);
    int i, len = RARRAY_LENINT((flatten_child_indexes));
    VALUE ary = rb_ivar_get(self, id_iv_pool);
    for (i = 0; i < len; ++i) { rb_ary_delete_at(ary, RARRAY_PTR(flatten_child_indexes)[i]); }
    return self;
}

static VALUE scene_size(VALUE self) {
    VALUE ary = rb_ivar_get(self, id_iv_pool);
    return rb_int_new(RARRAY_LENINT(ary));
}

static VALUE scene_get_child(VALUE self, VALUE index) {
    VALUE ary = rb_ivar_get(self, id_iv_pool);
    return rb_ary_entry(ary, NUM2INT(index));    
}

static VALUE scene_hit(int argc, VALUE argv[], VALUE self) {
    VALUE target, event_able;
    rb_scan_args(argc, argv, "11", &target, &event_able);

    volatile VALUE result = rb_hash_new();
    BOOL hitFlag = FALSE;
    VALUE scene_class = rb_path2class("SDLUdon::Scene");
    BOOL target_is_scene = (rb_obj_is_kind_of(target, scene_class) == Qtrue);
    int i, j;
    VALUE self_pool = rb_ivar_get(self, rb_intern("@pool"));
    int selfLen = RARRAY_LENINT(self_pool);
    if (target_is_scene) {
        VALUE target_pool = rb_ivar_get(target, rb_intern("@pool"));
        int targetLen = RARRAY_LENINT(target_pool);
        for (i = 0; i < selfLen; ++i) {
            for (j = 0; j < targetLen; ++j) {
                VALUE a = RARRAY_PTR(self_pool)[i], b = RARRAY_PTR(target_pool)[j];
                volatile VALUE hit_data = rb_funcall(a, rb_intern("hit"), 1, b);
                if (!NIL_P(hit_data)) {
                    hitFlag = TRUE;
                    VALUE child_info = rb_hash_lookup(result, a);
                    if (NIL_P(child_info)) { child_info = rb_hash_aset(result, a, rb_hash_new()); }
                    rb_hash_aset(child_info, b, hit_data);
                }
            }
        }
    } else {
        for (i = 0; i < selfLen; ++i) {
            VALUE self_child = RARRAY_PTR(self_pool)[i];
            volatile VALUE hit_data = rb_funcall(self_child, rb_intern("hit"), 1, target);
            if (!NIL_P(hit_data)) {
                hitFlag  = TRUE; rb_hash_aset(result, self_child, hit_data);
            }
        }
    }
    if (!hitFlag) return Qnil;
    VALUE event = rb_ivar_get(self, rb_intern("@event_manager"));
    if (event_able == Qtrue) { rb_funcall(event, rb_intern("trigger"), 2, ID2SYM(rb_intern("hit")), result); }
    if (rb_block_given_p()) { rb_yield(result); return self; }

    return result;
}

static VALUE hit_response_block(VALUE data, VALUE self, int argc, VALUE argv[]) {
    volatile VALUE pair = RARRAY_PTR(data)[0];
    volatile VALUE hit_result =  RARRAY_PTR(data)[1];
    VALUE a_event = rb_ivar_get(RARRAY_PTR(pair)[0], rb_intern("@event_manager"));
    if (!NIL_P( a_event )) { rb_funcall(a_event, rb_intern("trigger"), 2, ID2SYM(rb_intern("hit")), RARRAY_PTR(hit_result)[0]); }
    VALUE b_event = rb_ivar_get(RARRAY_PTR(pair)[1], rb_intern("@event_manager"));
    if (!NIL_P( b_event )) { rb_funcall(b_event, rb_intern("trigger"), 2, ID2SYM(rb_intern("hit")), RARRAY_PTR(hit_result)[1]); }
    if (rb_block_given_p()) { rb_yield(hit_result); }
    return Qnil;
}

static VALUE scene_each_child_hit_block(VALUE child, VALUE self, int argc, VALUE argv[]) {
    if (rb_respond_to(child, rb_intern("each_hit"))) { rb_funcall(child, rb_intern("each_hit"), 0, NULL); }
    return Qnil;
}

static VALUE scene_each_hit(VALUE self) {
    if (rb_obj_is_kind_of(self, rb_path2class("SDLUdon::Collision")) == Qtrue) {
        VALUE pool = rb_ivar_get(self, rb_intern("@pool"));
        int i, j, len = RARRAY_LENINT(pool);
        volatile VALUE hit_info_ary = rb_hash_new();
        VALUE collision_module = rb_path2class("SDLUdon::Collision"); 
        for (i = 0; i < len; ++i) {
            for (j=i+1;j < len;++j) {
                VALUE a = RARRAY_PTR(pool)[i]; VALUE b = RARRAY_PTR(pool)[j];
                volatile VALUE result = rb_funcall(collision_module, rb_intern("hit_actor"), 2, a, b);
                if (!NIL_P(result)) { rb_hash_aset(hit_info_ary, rb_assoc_new(a, b), result); }
            }
        }
        rb_block_call(hit_info_ary, rb_intern("each"), 0, NULL, hit_response_block, self);
        rb_block_call(pool, rb_intern("each"), 0, NULL, scene_each_child_hit_block, self);
    }
    return self;
}

static VALUE scene_to_a(VALUE self) {
    volatile VALUE result_ary = rb_ary_new();
    VALUE pool = rb_ivar_get(self, rb_intern("@pool"));
    int i, poolLen = RARRAY_LENINT(pool);
    for (i = 0; i < poolLen; ++i) {
        VALUE child = RARRAY_PTR(pool)[i];
        volatile VALUE child_hit_area = rb_funcall(child, rb_intern("to_a"), 0);
        if (RB_TYPE_P(RARRAY_PTR(child_hit_area)[0], T_ARRAY)) {
            rb_ary_concat(result_ary, rb_funcall(child_hit_area, rb_intern("flatten"), 0) );
        } else {
            rb_ary_push(result_ary, child_hit_area);
        }
    }
    return result_ary;
}

static VALUE scene_swap_child(VALUE self, VALUE dst_scene) {
    volatile VALUE self_child = rb_ivar_get(self, id_iv_pool);
    volatile VALUE dst_child = rb_ivar_get(dst_scene, id_iv_pool);
    int i, len;
    if (rb_block_given_p()) {
        volatile VALUE childs = rb_funcall_with_block(self_child, rb_intern("find_all"), 0, NULL, rb_block_proc());
        len = RARRAY_LENINT(childs);
        for (i = 0; i < len; ++i) {
            rb_ary_push(dst_child, RARRAY_PTR(childs)[i]); 
            rb_ary_delete(self_child, RARRAY_PTR(childs)[i]);
        }
    } else {
        len = RARRAY_LENINT(self_child);
        for (i = 0; i < len; ++i) { rb_ary_push(dst_child, RARRAY_PTR(self_child)[i]); }
        rb_ary_clear(self_child);
    }
    return Qnil;
}

static VALUE scene_temp_child(int argc, VALUE argv[], VALUE self) {
    rb_need_block();    
    volatile VALUE childs; rb_scan_args(argc, argv, "*", &childs);
    rb_funcall(self, rb_intern("add_child"), 1, childs);
    rb_yield(Qnil);
    rb_funcall(self, rb_intern("delete_child"), 1, childs);
    return self;
}

static VALUE scene_each_hide( VALUE child, VALUE none, int argc, VALUE argv[]) {
    rb_funcall(child, rb_intern("hide"), 0);
    return Qnil;
}

static VALUE scene_hide(VALUE self) {
    rb_call_super(0, NULL);
    rb_block_call(rb_ivar_get(self, rb_intern("@pool")), rb_intern("each"), 
        0, NULL, scene_each_hide, Qnil);
    return self;
}

static VALUE scene_each_show( VALUE child, VALUE none, int argc, VALUE argv[]) {
    rb_funcall(child, rb_intern("show"), 0);
    return Qnil;
}

static VALUE scene_show(VALUE self) {
    rb_call_super(0, NULL);
    rb_block_call(rb_ivar_get(self, rb_intern("@pool")), rb_intern("each"), 
        0, NULL, scene_each_show, Qnil);
    return self;

}

static VALUE scene_is_visible(VALUE self) { return rb_call_super(0, NULL); }

static VALUE scene_each_move( VALUE item, VALUE point, int argc, VALUE argv[]) {
    VALUE diff_x = RARRAY_PTR(point)[0];
    VALUE diff_y = RARRAY_PTR(point)[1];
    ActorData* actor; Data_Get_Struct(item, ActorData, actor);
    rb_funcall(item, rb_intern("move"), 2, rb_funcall(diff_x, rb_intern("+"), 1, actor->x),
        rb_funcall(diff_y, rb_intern("+"), 1, actor->y));
    return Qnil;
}

static VALUE scene_move(VALUE self, VALUE x, VALUE y) {
    ActorData *scene; Data_Get_Struct(self, ActorData, scene);
    VALUE child = rb_ivar_get(self, id_iv_pool);
    volatile VALUE diff_x = rb_funcall(x, rb_intern("-"), 1, scene->x);
    volatile VALUE diff_y = rb_funcall(y, rb_intern("-"), 1, scene->y);
    rb_block_call(child, id_each, 0, NULL, scene_each_move, rb_assoc_new(diff_x, diff_y));
    rb_call_super(2, (VALUE[]){x, y});
    return self;
}

static VALUE scene_each_move_add( VALUE item, VALUE point, int argc, VALUE argv[]) {
    VALUE diff_x = RARRAY_PTR(point)[0];
    VALUE diff_y = RARRAY_PTR(point)[1];
    rb_funcall(item, rb_intern("move_add"), 2, diff_x, diff_y);
    return Qnil;
}

static VALUE scene_move_add(int argc, VALUE argv[], VALUE self) {
    ActorData *scene; Data_Get_Struct(self, ActorData, scene);
    VALUE old_x = scene->x;
    VALUE old_y = scene->y;
    rb_call_super(argc, argv);
    volatile VALUE diff_x = rb_funcall(scene->x, rb_intern("-"), 1, old_x);
    volatile VALUE diff_y = rb_funcall(scene->y, rb_intern("-"), 1, old_y);
    VALUE child = rb_ivar_get(self, id_iv_pool);
    rb_block_call(child, id_each, 0, NULL, scene_each_move_add, rb_assoc_new(diff_x, diff_y));
    return self;
}

static VALUE scene_each_event_trigger(int argc, VALUE argv[], VALUE self) { return Qnil; }

static VALUE scene_each_pause( VALUE child, VALUE none, int argc, VALUE argv[]) {
    rb_funcall(child, rb_intern("pause"), 0);
    return Qnil;
}

static VALUE scene_pause(VALUE self) {
    rb_call_super(0, NULL);
    rb_block_call(rb_ivar_get(self, rb_intern("@pool")), rb_intern("each"), 
        0, NULL, scene_each_pause, Qnil);
    return self;
}

static VALUE scene_each_resume( VALUE child, VALUE none, int argc, VALUE argv[]) {
    rb_funcall(child, rb_intern("resume"), 0);
    return Qnil;
}

static VALUE scene_resume(VALUE self) {
    rb_call_super(0, NULL);
    rb_block_call(rb_ivar_get(self, rb_intern("@pool")), rb_intern("each"), 
        0, NULL, scene_each_resume, Qnil);
    return self;
}


void Init_scene(VALUE parent_module) {
    VALUE actor_class = rb_path2class("SDLUdon::Actor");
    VALUE scene_class = rb_define_class_under(parent_module, "Scene", actor_class);
    rb_define_private_method(scene_class, "initialize", scene_initialize, -1);
    rb_define_method(scene_class, "size", scene_size, 0);
    rb_define_method(scene_class, "add_child", scene_add_child, -1);
    rb_define_method(scene_class, "<<", scene_add_child, -1); //_syntax_sugar, 1);
    rb_define_method(scene_class, "[]", scene_get_child, 1);
    rb_define_method(scene_class, "child_at", scene_get_child, 1);
    rb_define_method(scene_class, "draw", scene_draw, -1);
    rb_define_method(scene_class, "update", scene_update, 0);
    rb_define_method(scene_class, "each_child", scene_each_child, 0);
    rb_define_method(scene_class, "swap_child", scene_swap_child, 1);
    rb_define_method(scene_class, "temp_child", scene_temp_child, -1);
    rb_define_method(scene_class, "find_child", scene_find_child, -1);
    rb_define_method(scene_class, "find_all_child", scene_find_children, -1);
    rb_define_method(scene_class, "delete_child", scene_delete_child, -1);
    rb_define_method(scene_class, "delete_child_at", scene_delete_child_at, -1);
    rb_define_method(scene_class, "clear_child", scene_clear_child, 0);
    rb_define_method(scene_class, "sort_child", scene_sort_child, 0);
    rb_define_method(scene_class, "unshift_child", scene_unshift_child, -1);
    rb_define_method(scene_class, "each_event_trigger", scene_each_event_trigger, -1);
    rb_define_method(scene_class, "hit", scene_hit, -1);
    rb_define_method(scene_class, "each_hit", scene_each_hit, 0);
    rb_define_method(scene_class, "to_a", scene_to_a, 0);
    rb_define_method(scene_class, "hide", scene_hide, 0);
    rb_define_method(scene_class, "show", scene_show, 0);
    rb_define_method(scene_class, "visible?", scene_is_visible, 0);
    rb_define_method(scene_class, "move", scene_move, 2);
    rb_define_method(scene_class, "move_add", scene_move_add, -1);
    rb_define_method(scene_class, "pause", scene_pause, 0);
    rb_define_method(scene_class, "resume", scene_resume, 0);

    rb_define_attr(scene_class, "screen", 1, 1);
}
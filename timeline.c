#include "sdl_udon_private.h"

static void tween_actor_move(VALUE self, VALUE move_event_data) {
    VALUE parent = rb_ivar_get(self, rb_intern("@parent"));
    volatile VALUE move_event_data_ary = rb_funcall(move_event_data, rb_intern("to_a"), 0);
    int i, len = RARRAY_LENINT(move_event_data_ary);
    for (i = 0; i < len; ++i) {
      VALUE pair = RARRAY_PTR(move_event_data_ary)[i];
      VALUE key_str = rb_sym_to_s(rb_ary_entry(pair, 0));
      VALUE val = rb_hash_lookup(rb_ary_entry(pair, 1), ID2SYM(rb_intern("current_num")));
      rb_funcall(parent, rb_intern_str(rb_str_cat2(key_str, "=")), 1, val);
    }
}

static VALUE tween_move_to_complete_block(VALUE val, VALUE self, int argc, VALUE argv[]) {
    tween_actor_move(self, argv[0]);
    return Qnil;
}

static VALUE tween_move_by_complete_block(VALUE val, VALUE self, int argc, VALUE argv[]) {
    tween_actor_move(self, argv[0]);
    return Qnil;
}

static VALUE tween_move_to_enterframe_block(VALUE val, VALUE self, int argc, VALUE argv[]) {
     tween_actor_move(self, argv[0]);
    return Qnil;
}

static VALUE tween_move_by_enterframe_block(VALUE val, VALUE self, int argc, VALUE argv[]) {
     tween_actor_move(self, argv[0]);
    return Qnil;
}

static VALUE timeline_move_body(VALUE dummy, VALUE param_hash, int argc, VALUE argv[]) {
    VALUE self = argv[0];
    VALUE vector = rb_hash_lookup(param_hash, ID2SYM(rb_intern("vector") ));
    VALUE data = rb_iv_get(self, "@data");
    VALUE duration = rb_hash_lookup(param_hash, ID2SYM(rb_intern("duration") ));
    VALUE easing = rb_hash_lookup(param_hash, ID2SYM(rb_intern("easing") ));
    VALUE move_type = rb_hash_lookup(param_hash, ID2SYM(rb_intern("move_type") ));
    volatile VALUE vector_ary = rb_funcall(vector, rb_intern("to_a"), 0);
    int i, len = RARRAY_LENINT(vector_ary);
    for (i = 0; i < len; ++i) {
      VALUE pair = RARRAY_PTR(vector_ary)[i];
      VALUE key_str = rb_funcall(RARRAY_PTR(pair)[0], rb_intern("to_s"),0); 
      const char* key = StringValuePtr(key_str);
      VALUE val = RARRAY_PTR(pair)[1];
      if (RB_TYPE_P(val, T_SYMBOL))
        rb_hash_aset(vector, ID2SYM(rb_intern(key)), rb_hash_lookup(data, ID2SYM(rb_intern(key))) );
    }

    volatile VALUE tween_instance = rb_obj_alloc(rb_path2class("SDLUdon::Tween"));
    VALUE parent = rb_ivar_get(self, rb_intern("@parent"));

    volatile VALUE tween_hash = rb_hash_new();
    for (i = 0; i < len; ++i) {
      VALUE pair = RARRAY_PTR(vector_ary)[i];
      VALUE key_str = rb_funcall(RARRAY_PTR(pair)[0], rb_intern("to_s"),0); 
      const char* key = StringValuePtr(key_str);
      VALUE val = RARRAY_PTR(pair)[1];
      volatile VALUE init_v;
      if (rb_respond_to(parent, rb_intern(key))) {
        init_v = rb_funcall(parent, rb_intern(key), 0);
      } else {
        volatile VALUE init_v_key = rb_str_cat2(rb_str_new2("s"), key );
        init_v = rb_hash_lookup(vector, rb_funcall(init_v_key, rb_intern("to_sym"), 0));
        if (NIL_P(init_v)) init_v = rb_int_new(0); // 暫定。originを採用すればそちらを使う。
      }
      volatile VALUE v_hash = rb_hash_new();
      rb_hash_aset(tween_hash, ID2SYM(rb_intern(key)), v_hash);
      rb_hash_aset(v_hash, ID2SYM(rb_intern("init_num")), init_v);
      if (rb_str_cmp(move_type, rb_str_new2("by")) == Qtrue) {
        rb_hash_aset(v_hash, ID2SYM(rb_intern("goal_num")), rb_funcall(val, rb_intern("+"), 1, init_v));
      } else {
        rb_hash_aset(v_hash, ID2SYM(rb_intern("goal_num")), val);
      }
    }
    rb_obj_call_init(tween_instance, 3, (VALUE[]) { tween_hash, duration, easing });
    if (!NIL_P(parent)) {
      volatile VALUE tween_event_manager = rb_funcall(tween_instance, rb_intern("event"),0);
      
      VALUE (*complete_block_handler)(VALUE, VALUE, int, VALUE[]);
      VALUE (*enterframe_block_handler)(VALUE, VALUE, int, VALUE[]);
      if (rb_str_cmp(move_type, rb_str_new2("by")) == Qtrue) {
        complete_block_handler = tween_move_by_complete_block;
        enterframe_block_handler = tween_move_by_enterframe_block;
      } else {
        complete_block_handler = tween_move_to_complete_block;
        enterframe_block_handler = tween_move_to_enterframe_block;
      }
      rb_block_call(tween_event_manager, rb_intern("add"), 1, (VALUE []) { ID2SYM(rb_intern("complete")) }, complete_block_handler, self);
      rb_block_call(tween_event_manager, rb_intern("add"), 1, (VALUE []) { ID2SYM(rb_intern("enterframe")) }, enterframe_block_handler, self);
    }
    rb_hash_aset(param_hash, ID2SYM(rb_intern("action")), tween_instance); 
    return Qnil;  
}

static VALUE timeline_move(int argc, VALUE argv[], VALUE self, const char* moveType) {
    volatile VALUE x_or_hash, y, duration, easing;
    int scan_argc = rb_scan_args(argc, argv, "13", &x_or_hash, &y, &duration, &easing);
    volatile VALUE hash = rb_hash_new();
    rb_hash_aset(hash, ID2SYM(rb_intern("vector")), rb_hash_new());
    VALUE vector = rb_hash_lookup(hash, ID2SYM(rb_intern("vector")));
    if (RB_TYPE_P(x_or_hash, T_HASH)) {
      volatile VALUE x_or_hash_ary = rb_funcall(x_or_hash, rb_intern("to_a"), 0);
      int i, len = RARRAY_LENINT(x_or_hash_ary);
      for (i = 0; i < len; ++i) {
        VALUE pair = RARRAY_PTR(x_or_hash_ary)[i];
        VALUE key_str = rb_funcall(RARRAY_PTR(pair)[0], rb_intern("to_s"),0); 
        const char* key = StringValuePtr(key_str);
        VALUE val = RARRAY_PTR(pair)[1];
        rb_hash_aset(vector, ID2SYM(rb_intern(key)), val);
      }
      switch (scan_argc) {
        case 1:
          duration = rb_float_new(1.0);
          easing = ID2SYM(rb_intern("none"));
          break;
        case 2:
          duration = y;
          easing = ID2SYM(rb_intern("none"));
          break;       
      }
    } else {
      rb_hash_aset(vector, ID2SYM(rb_intern("x")), x_or_hash);
      rb_hash_aset(vector, ID2SYM(rb_intern("y")), y);
      switch (scan_argc) {
        case 2:
          duration = rb_float_new(1.0);
          easing = ID2SYM(rb_intern("none"));
          break;
        case 3:
          easing = ID2SYM(rb_intern("none"));
          break;       
      }
    }
    rb_hash_aset(hash, ID2SYM(rb_intern("duration")), duration);
    rb_hash_aset(hash, ID2SYM(rb_intern("easing")), easing);

    int moveTypeLen = strlen(moveType);
    if (strncmp(moveType, "by", moveTypeLen) == 0) {
      rb_hash_aset(hash, ID2SYM(rb_intern("move_type")), rb_str_new2("by"));
    } else if (strncmp(moveType, "to", moveTypeLen) == 0) {
      rb_hash_aset(hash, ID2SYM(rb_intern("move_type")), rb_str_new2("to"));
    }
    rb_hash_aset(hash, ID2SYM(rb_intern("create_action")), rb_proc_new(timeline_move_body, hash));
    if ( RARRAY_LENINT( rb_ivar_get(self, rb_intern("@pool")) ) == 0 ) {
        timeline_move_body(self, hash, 1 , (VALUE[]) { self });
    }
    rb_ary_push(rb_ivar_get(self, rb_intern("@pool")), hash);
    return self;
}

static VALUE timeline_move_to(int argc, VALUE argv[], VALUE self) {
    return timeline_move(argc, argv, self, "to");
}

static VALUE timeline_move_by(int argc, VALUE argv[], VALUE self) {
    return timeline_move(argc, argv, self, "by");
}

static VALUE timeline_action(int argc, VALUE argv[], VALUE self) {
  VALUE proc; rb_scan_args(argc, argv, "01", &proc);
  if (NIL_P(proc)) {
    rb_need_block();
    proc = rb_block_proc();
  } else {
    if (rb_funcall(rb_cProc, rb_intern("=="), 1, rb_class_of(proc)) == Qfalse) {
      rb_raise(rb_eException, "Need proc object!");  
    }
  }

  volatile VALUE hash = rb_hash_new();
  rb_hash_aset(hash, ID2SYM(rb_intern("action")), proc);
  rb_ary_push(rb_ivar_get(self, rb_intern("@pool")), hash);
  return self;
}

static VALUE timeline_loop(VALUE self) {
    TimeLineData *timeline; Data_Get_Struct(self, TimeLineData, timeline);
    timeline->looped = TRUE;
    return self;
}

static VALUE timeline_end(VALUE self) {
    TimeLineData *timeline; Data_Get_Struct(self, TimeLineData, timeline);
    timeline->looped = FALSE;
    return self;
}

static VALUE timeline_delay_body(VALUE dummy, VALUE param_hash, int argc, VALUE argv[]) {
    volatile VALUE delay = rb_hash_lookup(param_hash, ID2SYM(rb_intern("delay")));
    volatile VALUE delay_count = rb_hash_lookup(param_hash, ID2SYM(rb_intern("delay_count")));
    int c_delay_count = NUM2INT(delay_count);
    delay_count = rb_int_new(++c_delay_count);
    if (rb_funcall(delay, rb_intern("<="), 1, delay_count ) == Qtrue ) {
        rb_hash_aset(param_hash, ID2SYM(rb_intern("delay_count")), rb_int_new(0));        
        return Qfalse;
    } else {
        rb_hash_aset(param_hash, ID2SYM(rb_intern("delay_count")), delay_count);
        return Qtrue;
    }
}

static VALUE timeline_delay(VALUE self, VALUE delay) {
    volatile VALUE hash = rb_hash_new();
    rb_hash_aset(hash, ID2SYM(rb_intern("delay")), delay);
    rb_hash_aset(hash, ID2SYM(rb_intern("delay_count")), rb_int_new(0));
    rb_hash_aset(hash, ID2SYM(rb_intern("action")), rb_proc_new(timeline_delay_body, hash)); 
    rb_ary_push(rb_ivar_get(self, rb_intern("@pool")), hash);
    return self;
}

static VALUE timeline_alloc(VALUE klass) {
    TimeLineData *timeline = ALLOC(TimeLineData);
    return Data_Wrap_Struct(klass, 0, -1, timeline);
}

static VALUE timeline_initialize(int argc, VALUE argv[], VALUE self) {
    VALUE parent;
    rb_scan_args(argc, argv, "01", &parent);
    TimeLineData* timelineData; Data_Get_Struct(self, TimeLineData, timelineData);
    timelineData->looped = FALSE;
    rb_iv_set(self, "@pool", rb_ary_new());
    rb_iv_set(self, "@data", rb_hash_new());
    rb_ivar_set(self, rb_intern("@parent"), parent);
    VALUE event_manager = rb_class_new_instance(0, NULL, rb_path2class("SDLUdon::Event::Manager"));
    rb_ivar_set(self, rb_intern("@event_manager"), event_manager);

    return Qnil;
}

static VALUE timeline_is_empty(VALUE self) {
    volatile VALUE ary = rb_ivar_get(self, id_iv_pool);
    return rb_funcall(ary, rb_intern("empty?"), 0);
}

static VALUE timeline_is_active(VALUE self) {
    volatile VALUE ary = rb_ivar_get(self, id_iv_pool);
    return rb_funcall(ary, rb_intern("empty?"), 0) == Qtrue ? Qfalse : Qtrue;
}

static VALUE timeline_update(VALUE self) {
    volatile VALUE ary = rb_ivar_get(self, id_iv_pool);
    if (RARRAY_LENINT(ary) != 0) {
        volatile VALUE current_action_data = rb_ary_entry(ary, 0);
        volatile VALUE action = rb_hash_lookup(current_action_data, ID2SYM(rb_intern("action") ));
        VALUE result = rb_funcall(action, rb_intern("call"), 1, rb_iv_get(self, "@data"));
        if ( rb_respond_to(action, rb_intern("active?")) != 0)
            result = rb_funcall(action, rb_intern("active?"), 0); 
        if ( result != Qtrue ) {
            TimeLineData *timeline; Data_Get_Struct(self, TimeLineData, timeline);
            rb_ary_shift(ary);
            if (timeline->looped) rb_ary_push(ary, current_action_data);
            if (RARRAY_LENINT(ary) != 0) {
                VALUE next_action_data = rb_ary_entry(ary, 0);
                volatile VALUE create_action_proc = rb_hash_lookup(next_action_data, ID2SYM(rb_intern("create_action")));
                if ( !NIL_P(create_action_proc) ) rb_proc_call(create_action_proc, rb_ary_new4(1, (VALUE []) { self }) );
            } else {
              VALUE event = rb_ivar_get(self, rb_intern("@event_manager"));
              rb_funcall(event, rb_intern("trigger"), 1, ID2SYM(rb_intern("complete")));
            }
        }
    }
    return self;
}

static VALUE timeline_current(VALUE self) {
    return RARRAY_PTR(rb_ivar_get(self, id_iv_pool))[0];
}

static VALUE timeline_clear(VALUE self) {
    rb_ary_clear(rb_ivar_get(self, id_iv_pool));
    return self;
}

static VALUE timeline_get_event_manager(VALUE self) {
    return rb_ivar_get(self, rb_intern("@event_manager"));
}


void Init_timeline(VALUE parent) {
    VALUE timeline_class = rb_define_class_under(parent, "Timeline", rb_cObject);
    rb_define_alloc_func(timeline_class, timeline_alloc);
    rb_define_private_method(timeline_class, "initialize", timeline_initialize, -1);
    rb_define_method(timeline_class, "update", timeline_update, 0);
    rb_define_method(timeline_class, "move_to", timeline_move_to, -1);
    rb_define_method(timeline_class, "move_by", timeline_move_by, -1);
    rb_define_method(timeline_class, "action", timeline_action, -1);
    rb_define_method(timeline_class, "loop", timeline_loop, 0);
    rb_define_method(timeline_class, "end", timeline_end, 0);
    rb_define_method(timeline_class, "delay", timeline_delay, 1);
    rb_define_method(timeline_class, "current", timeline_current, -1);
    rb_define_method(timeline_class, "clear", timeline_clear, 0);
    rb_define_method(timeline_class, "empty?", timeline_is_empty, 0);
    rb_define_method(timeline_class, "active?", timeline_is_active, 0);
    rb_define_method(timeline_class, "event", timeline_get_event_manager, 0);
}
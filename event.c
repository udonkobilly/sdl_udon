#include "sdl_udon_private.h"

static st_table* cronPool = NULL;

static int CronEachUpdate(st_data_t key, st_data_t value, st_data_t arg) {
    VALUE cron = (VALUE)value;
    rb_funcall(cron, rb_intern("update"), 0);
    return ST_CONTINUE;
}

inline void Update_event() {
    st_foreach_safe(cronPool, CronEachUpdate, (st_data_t)NULL);
}

static VALUE module_event_add_timer(int argc, VALUE argv[], VALUE self) {
    volatile VALUE timer = rb_class_new_instance(argc, argv, rb_path2class("SDLUdon::Event::Cron"));
    st_insert(cronPool, NUM2INT(rb_obj_id(timer)), (st_data_t)timer);
    return timer;
}

static VALUE module_event_delete_timer(VALUE self, VALUE obj_or_id) {
    Uint32 key = rb_type_p(obj_or_id, T_FIXNUM) ? NUM2INT(obj_or_id) : NUM2INT(rb_obj_id(obj_or_id));
    st_data_t data;;
    st_delete(cronPool, (st_data_t*)&key, &data);
    return data;
}

static VALUE module_event_clear_timer(VALUE self) {
    st_clear(cronPool);
    return Qnil;
}

static void cron_mark(CronData *cronData) {
    rb_gc_mark(cronData->proc);
    rb_gc_mark(cronData->counter);
}

static VALUE cron_alloc(VALUE self) {
    CronData* cronData = ALLOC(CronData);
    return Data_Wrap_Struct(self, cron_mark, -1, cronData);
}

static void cron_reset(int argc, VALUE argv[], VALUE self) {
    VALUE interval, proc, loop;
    if (rb_block_given_p()) {
        rb_scan_args(argc, argv, "02&", &interval, &loop, &proc);
    } else {
        rb_scan_args(argc, argv, "03",  &interval, &proc, &loop);
    }
    CronData* cronData; Data_Get_Struct(self, CronData, cronData);
    if (!NIL_P(interval)) cronData->interval = NUM2UINT(interval);
    if (!NIL_P(loop)) cronData->loop = NUM2INT(loop);
    if (!NIL_P(proc)) cronData->proc = proc;
    cronData->loop_count = 0;
    cronData->paused = FALSE;
}

static VALUE cron_initialize(int argc, VALUE argv[], VALUE self) {
    VALUE interval, proc; 
    volatile VALUE loop;
    if (rb_block_given_p()) {
        rb_scan_args(argc, argv, "11&", &interval, &loop, &proc);
    } else {
        rb_scan_args(argc, argv, "21",  &interval, &proc, &loop);
    }
    if (NIL_P(loop)) loop = rb_int_new(-1);
    CronData* cronData; Data_Get_Struct(self, CronData, cronData);
    cronData->interval = NUM2UINT(interval);
    cronData->loop = NUM2INT(loop);
    cronData->loop_count = 0;
    cronData->proc = proc;
    cronData->counter = rb_class_new_instance(0, NULL, rb_path2class("SDLUdon::Counter::Clock"));
    return Qnil;
}

static VALUE cron_update(VALUE self) {
    CronData *cronData;Data_Get_Struct(self, CronData, cronData);
    if (cronData->paused) return Qnil;
    Uint32 ticks = NUM2INT(rb_funcall(cronData->counter, rb_intern("now"), 0));
    Uint32 interval = cronData->interval;

    if (ticks > interval) {
        rb_funcall(cronData->counter, rb_intern("restart"), 0);  
        rb_funcall(cronData->proc, rb_intern("call"), 0);
        if (0 > --cronData->loop_count) rb_funcall(self, rb_intern("kill"), 0);
    }
    return Qnil;
}

static VALUE cron_kill(VALUE self) {
    VALUE class_event = rb_path2class("SDLUdon::Event");
    rb_funcall(class_event, rb_intern("delete_timer"), 1, self);
    return Qnil;
}

static VALUE cron_counter(VALUE self) {
    CronData *cronData;Data_Get_Struct(self, CronData, cronData);
    return cronData->counter;
}

static VALUE cron_restart(int argc, VALUE argv[], VALUE self) {
    cron_reset(argc, argv, self);
    CronData *cronData;Data_Get_Struct(self, CronData, cronData);
    rb_funcall(cronData->counter, rb_intern("restart"), 0);
    return self;
}

static VALUE cron_suspend(VALUE self) {
    CronData *cronData;Data_Get_Struct(self, CronData, cronData);
    cronData->paused = TRUE;
    rb_funcall(cronData->counter, rb_intern("pause"), 0);
    return self;
}

static VALUE cron_resume(VALUE self) {
    CronData *cronData;Data_Get_Struct(self, CronData, cronData);
    cronData->paused = FALSE;
    rb_funcall(cronData->counter, rb_intern("resume"), 0);
    return self;
}

static VALUE event_manager_create(int argc, VALUE argv[], VALUE self) {
    volatile VALUE event_name_sym, event_block;
    rb_scan_args(argc, argv, "2", &event_name_sym, &event_block);
    return event_block;
}

static VALUE event_manager_is_key(VALUE self, VALUE event_name_sym) {
    volatile VALUE sym = rb_funcall( rb_funcall(event_name_sym, rb_intern("to_s"), 0, NULL), rb_intern("to_sym"), 0, NULL);
    volatile VALUE ary = rb_hash_lookup(rb_ivar_get(self, rb_intern("@pool")), sym);
    return NIL_P(ary) ? Qfalse : Qtrue;
}

static VALUE AddEvent(VALUE self, VALUE event_name, VALUE event_block) {
    event_name = rb_funcall( rb_funcall(event_name, rb_intern("to_s"), 0, NULL), rb_intern("to_sym"), 0, NULL);
    volatile VALUE hash = rb_ivar_get(self, rb_intern("@pool"));
    volatile VALUE ary = rb_hash_lookup(hash, event_name);
    if ( NIL_P(ary) )  { ary = rb_ary_new(); rb_hash_aset(hash, event_name, ary); }
    volatile VALUE result_proc = event_manager_create(2, (VALUE []){ event_name, event_block  }, self);
    rb_ary_push(ary, result_proc);
    return rb_obj_id(result_proc);
}


static VALUE event_manager_add(int argc, VALUE argv[], VALUE self) {
    volatile VALUE event_name_or_hash, event_handler, event_block; 
    rb_scan_args(argc, argv, "11&", &event_name_or_hash, &event_handler, &event_block);
    int i, handlersLen;
    volatile VALUE event_ary, pair, handlers;
    if (rb_type_p(event_name_or_hash, T_HASH)) {
        event_ary = rb_funcall(event_name_or_hash, rb_intern("to_a"), 0);
        int j, eventAryLen = RARRAY_LENINT(event_ary);
        for (i = 0; i < eventAryLen; ++i) {
            pair = RARRAY_PTR(event_ary)[i];
            handlers = rb_funcall(rb_cArray, rb_intern("[]"), 1, RARRAY_PTR(pair)[1]);
            handlersLen = RARRAY_LENINT(handlers);
            for (j = 0; j < handlersLen; ++j) { AddEvent(self, RARRAY_PTR(pair)[0], RARRAY_PTR(handlers)[j]); }
        }
    } else if (!NIL_P(event_handler)) {
        handlers = rb_funcall(rb_cArray, rb_intern("[]"), 1, event_handler);
        handlersLen = RARRAY_LENINT(handlers);
        for (i = 0; i < handlersLen; ++i) { AddEvent(self, event_name_or_hash, RARRAY_PTR(handlers)[i]); }
    } else {
        rb_need_block();
        AddEvent(self, event_name_or_hash, event_block);
    }
    return self;
}

static VALUE event_manager_add_syntax_sugar(int argc, VALUE argv[], VALUE self) {
    VALUE event_name_sym, block;
    rb_scan_args(argc, argv, "2", &event_name_sym, &block);
    rb_funcall_with_block(self, rb_intern("add"), 1, (VALUE[]){ event_name_sym }, block);
    return Qnil;
}

static VALUE event_ary_delete_if(VALUE val, VALUE target_id, int argc, VALUE argv[]) {
    return rb_funcall(rb_obj_id(val), rb_intern("=="), 1, target_id);
}

static VALUE event_ary_find_all(VALUE val, VALUE target_id, int argc, VALUE argv[]) {
    return rb_funcall(rb_obj_id(val), rb_intern("=="), 1, target_id);
}


static VALUE event_manager_delete(int argc, VALUE argv[], VALUE self) {
    VALUE event_name_sym, target;
    rb_scan_args(argc, argv, "11", &event_name_sym, &target);
    VALUE hash = rb_ivar_get(self, rb_intern("@pool"));
    volatile VALUE ary = rb_hash_lookup(hash, event_name_sym);
    if (NIL_P(ary)) return Qnil;
    volatile VALUE delete_items = Qundef;
    if (NIL_P(target)) {
        if (rb_block_given_p()) {
            delete_items = rb_block_call(ary, rb_intern("find_all"), 0, NULL, event_ary_find_all, target);
            rb_block_call(ary, rb_intern("delete_if"), 0, NULL, event_ary_delete_if, target);
        } else {
          delete_items = rb_ary_dup(ary);
          rb_funcall(ary, rb_intern("clear"), 0);
        }
    } else if (FIXNUM_P(target)) {
        delete_items = rb_ary_delete_at(ary, NUM2INT(target));
    } else {
        delete_items = rb_ary_delete(ary, target);
    }
    return rb_funcall(rb_cArray, rb_intern("[]"), 1, delete_items);
}

static VALUE event_ary_each_call(VALUE val, VALUE event_data, int argc, VALUE argv[]) {
    rb_funcall(val, rb_intern("call"), 1, event_data);
    return Qnil;
}

static VALUE event_manager_trigger(int argc, VALUE argv[], VALUE self) {
    volatile VALUE event_name_sym, event_data;
    rb_scan_args(argc, argv, "11", &event_name_sym, &event_data);
    volatile VALUE ary = rb_hash_lookup(rb_ivar_get(self, id_iv_pool), event_name_sym);
    if (NIL_P(ary)) return Qnil;
    rb_block_call(ary, id_each, 0, NULL, event_ary_each_call, event_data);
    return self;
}


static VALUE event_manager_alloc(VALUE self) {    
    EventManagerData *eventManager = ALLOC(EventManagerData);
    return Data_Wrap_Struct(self, 0, -1, eventManager);
}

static VALUE event_manager_initialize(VALUE self) {
    rb_iv_set(self, "@pool", rb_hash_new());
    return Qnil;
}

static VALUE event_manager_clear(int argc, VALUE argv[], VALUE self) {
    volatile VALUE hash = rb_iv_get(self, "@pool");
    VALUE event_name_sym;
    if (rb_scan_args(argc, argv, "01", &event_name_sym) == 0) {
        rb_hash_clear(hash);
    } else {
        VALUE ary = rb_hash_lookup(hash, event_name_sym);
        if (!NIL_P(ary)) rb_ary_clear(ary);
    }
    return self;
}


void Quit_event() { st_free_table(cronPool); }


static VALUE event_manager_is_empty(int argc, VALUE argv[], VALUE self) {
    VALUE result = Qtrue;
    VALUE pool = rb_ivar_get(self, rb_intern("@pool"));
    VALUE event_name;
    rb_scan_args(argc, argv, "01", &event_name);
    if (NIL_P(event_name)) {
        volatile VALUE pool_ary = rb_funcall(pool, rb_intern("to_a"), 0);
        int poolAryLen = RARRAY_LENINT(pool_ary);
        for (int i = 0; i < poolAryLen; ++i) {
            if (rb_funcall(RARRAY_PTR(pool_ary)[i], rb_intern("any?"), 0) == Qtrue) { result = Qfalse; break; }
        }
    } else {
      volatile VALUE sym = rb_funcall( rb_funcall(event_name, rb_intern("to_s"), 0, NULL), rb_intern("to_sym"), 0, NULL);
      VALUE ary = rb_hash_lookup(pool, sym);
      if (!NIL_P(ary) && rb_funcall(ary, rb_intern("any?"), 0) == Qtrue ) { result = Qfalse; }
    }
    return result;
}


void Init_event(VALUE parent) {
    VALUE module_event = rb_define_module_under(parent, "Event");
    rb_define_module_function(module_event, "add_timer", module_event_add_timer, -1);
    rb_define_module_function(module_event, "delete_timer", module_event_delete_timer, -1);
    rb_define_module_function(module_event, "clear_timer", module_event_clear_timer, 0);

    VALUE class_cron = rb_define_class_under(module_event, "Cron", rb_cObject);
    rb_define_alloc_func(class_cron, cron_alloc);
    rb_define_private_method(class_cron, "initialize", cron_initialize, -1);
    rb_define_method(class_cron, "update", cron_update, 0);
    rb_define_method(class_cron, "kill", cron_kill, 0);
    rb_define_method(class_cron, "counter", cron_counter, 0);
    rb_define_method(class_cron, "restart", cron_restart, -1);
    rb_define_method(class_cron, "pause", cron_suspend, 0);
    rb_define_method(class_cron, "resume", cron_resume, 0);

    VALUE event_class = rb_define_class_under(module_event, "Manager", rb_cObject);
    rb_define_alloc_func(event_class, event_manager_alloc);
    rb_define_private_method(event_class, "initialize", event_manager_initialize, 0);
    rb_define_private_method(event_class, "create", event_manager_create, -1);
    rb_define_private_method(event_class, "key?", event_manager_is_key, 1);
    rb_define_method(event_class, "add", event_manager_add, -1);
    rb_define_method(event_class, "[]=", event_manager_add_syntax_sugar, -1);
    rb_define_method(event_class, "empty?", event_manager_is_empty, -1);
    rb_define_method(event_class, "delete", event_manager_delete, -1);
    rb_define_method(event_class, "clear", event_manager_clear, -1);
    rb_define_method(event_class, "trigger", event_manager_trigger, -1);
    cronPool = st_init_numtable();
}
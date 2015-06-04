#include "sdl_udon_private.h"

static VALUE state_machine_run(int argc, VALUE argv[], VALUE self) {
    StateMachineData *state_machine; Data_Get_Struct(self, StateMachineData, state_machine);
    if (state_machine->suspend) return self;
    VALUE args; rb_scan_args(argc, argv, "0*", &args);
    rb_fiber_resume(state_machine->fiber, 1, (VALUE[]) { args });
    return self;
}

static VALUE state_machine_fiber(VALUE data, VALUE self) {
    StateMachineData *stateMachine; Data_Get_Struct(self, StateMachineData, stateMachine);
    VALUE return_argv[1];
    const VALUE *argv = RARRAY_PTR(data);
    int argc = RARRAY_LENINT(data);
    while (TRUE) {
        stateMachine->changed = FALSE;
        return_argv[0] = rb_funcall2(stateMachine->run_method, rb_intern("call"), argc, argv);
        stateMachine->changed = FALSE;
        if (stateMachine->delay > 0) {
            stateMachine->delay--;
            if (stateMachine->delay <= 0) {
                stateMachine->delay = 0;
                rb_funcall(self, rb_intern("change"), 1, stateMachine->reserve_state_name);
                stateMachine->reserve_state_name = Qnil;
            }
        }
        rb_fiber_yield(1, return_argv);
    }
    return Qnil;
}

static VALUE state_machine_is_empty(VALUE self) {
    VALUE pool = rb_ivar_get(self, rb_intern("@pool"));
    return (NUM2INT(rb_funcall(pool, rb_intern("size"), 0)) == 0) ? Qtrue : Qfalse;
}

static VALUE state_machine_is_change(VALUE self) {
    StateMachineData *stateMachine; Data_Get_Struct(self, StateMachineData, stateMachine);
    return stateMachine->changed ? Qtrue : Qfalse;
}

static VALUE state_machine_get_member(VALUE self) {
    VALUE pool = rb_ivar_get(self, rb_intern("@pool"));
    return rb_funcall(pool, rb_intern("keys"), 0);
}

static VALUE state_machine_is_member(VALUE self, VALUE key) {
    VALUE pool = rb_ivar_get(self, rb_intern("@pool"));
    return NIL_P(rb_funcall(pool, rb_intern("[]"), 1, key)) ? Qfalse : Qtrue;
}

static VALUE state_machine_add(int argc, VALUE argv[], VALUE self) {
    VALUE pool = rb_ivar_get(self, rb_intern("@pool"));
    volatile VALUE key_or_hash, value, parent, block;
    rb_scan_args(argc, argv, "12&", &key_or_hash, &value, &parent, &block);
    if (rb_block_given_p()) {
        rb_hash_aset(pool, key_or_hash, block);
    } else {
        if (RB_TYPE_P(key_or_hash, T_HASH)) {
            volatile VALUE ary = rb_funcall(key_or_hash, rb_intern("to_a"), 0);
            int i, len = RARRAY_LENINT(ary);
            for (i = 0; i < len; ++i) {
                volatile VALUE pair = RARRAY_PTR(ary)[i];
                state_machine_add(2, (VALUE[]){ RARRAY_PTR(pair)[0], RARRAY_PTR(pair)[1] }, self);
            }
        } else {
            if (rb_obj_is_proc(value) == Qtrue) {
                rb_hash_aset(pool, key_or_hash, value);
            }  else {
                rb_hash_aset(pool, key_or_hash, rb_obj_method(NIL_P(parent) ? rb_ivar_get(self, rb_intern("@parent")) : parent, value));
            }
        }
    }
    return self;
}
static VALUE state_machine_change(int argc, VALUE argv[], VALUE self) {
    VALUE state, delay;
    rb_scan_args(argc, argv, "11", &state, &delay);
    volatile VALUE pool = rb_ivar_get(self, rb_intern("@pool"));
    StateMachineData *stateMachine; Data_Get_Struct(self, StateMachineData, stateMachine);
    volatile VALUE state_sym = rb_funcall(state, rb_intern("to_sym"), 0);
    if (rb_funcall(state_sym, rb_intern("=="), 1,rb_ivar_get(self, rb_intern("@state_name"))) == Qtrue)
        return Qnil;
    stateMachine->changed = TRUE;
    if (NIL_P(delay)) {
        rb_ivar_set(self, rb_intern("@old_state_name"), rb_ivar_get(self, rb_intern("@state_name")));
        rb_ivar_set(self, rb_intern("@state_name"), state_sym);
        stateMachine->run_method = rb_hash_lookup(pool, state_sym);
    } else {
        int cDelay = NUM2INT(delay);
        if (cDelay <= 0) return Qnil;
        stateMachine->delay = cDelay;
        stateMachine->reserve_state_name = state_sym;
        if (rb_block_given_p())
            stateMachine->run_method = rb_block_proc();
    }
    return self;
}

static VALUE state_machine_call(int argc, VALUE argv[], VALUE self) {
    VALUE state, args;
    rb_scan_args(argc, argv, "1*", &state, &args);
    volatile VALUE state_sym = rb_funcall(state, rb_intern("to_sym"), 0);
    VALUE pool = rb_ivar_get(self, rb_intern("@pool"));
    VALUE run_method = rb_hash_lookup(pool, state_sym);
    return rb_funcall(run_method, rb_intern("call"), argc-1, args);
}

static void state_machine_mark(StateMachineData *state_machine) {
    rb_gc_mark(state_machine->fiber);
    rb_gc_mark(state_machine->run_method);
    rb_gc_mark(state_machine->reserve_state_name);
}

static VALUE state_machine_empty_proc(VALUE dummy, VALUE param_hash, int argc, VALUE argv[]) { return Qnil; }

static VALUE state_machine_alloc(VALUE klass) {
    StateMachineData *state_machine = ALLOC(StateMachineData);
    state_machine->run_method = rb_proc_new(state_machine_empty_proc, Qnil);
    state_machine->reserve_state_name = Qnil;
    volatile VALUE self = Data_Wrap_Struct(klass, state_machine_mark, -1, state_machine);
    state_machine->fiber = rb_fiber_new(state_machine_fiber, self);
    return self;
}

static VALUE state_machine_pause(VALUE self) {
    StateMachineData *stateMachine; Data_Get_Struct(self, StateMachineData, stateMachine);
    stateMachine->suspend = TRUE;
    return self;
}


static VALUE state_machine_resume(VALUE self) {
    StateMachineData *stateMachine; Data_Get_Struct(self, StateMachineData, stateMachine);
    stateMachine->suspend = FALSE;
    return self;
}

static VALUE state_machine_none_proc(VALUE dummy, VALUE param_hash, int argc, VALUE argv[]) { return Qnil; }

static VALUE state_machine_initialize(int argc, VALUE argv[], VALUE self) {
    StateMachineData *stateMachine; Data_Get_Struct(self, StateMachineData, stateMachine);
    stateMachine->suspend = stateMachine->changed = FALSE;
    stateMachine->delay = 0;
    rb_ivar_set(self, rb_intern("@state_name"), ID2SYM(rb_intern("none")));
    rb_ivar_set(self, rb_intern("@pool"), rb_hash_new());
    VALUE parent;
    rb_scan_args(argc, argv, "01", &parent);
    rb_ivar_set(self, rb_intern("@parent"), parent);
    rb_funcall(self, rb_intern("add"), 2, ID2SYM(rb_intern("none")), rb_proc_new(state_machine_none_proc, Qnil));
    return Qnil;
}
static VALUE state_machine_register(int argc, VALUE *argv, VALUE klass) {
    volatile VALUE parent, method_sym;
    rb_scan_args(argc, argv, "02", &parent, &method_sym);
    if (NIL_P(method_sym)) method_sym = ID2SYM(rb_intern("run")); 
    volatile VALUE state_machine_obj = rb_obj_alloc(klass);
    rb_obj_call_init(state_machine_obj, 0, NULL);
    if (NIL_P(parent)) parent =  state_machine_obj;

    StateMachineData *state_machine; Data_Get_Struct(state_machine_obj, StateMachineData, state_machine);
    VALUE ary = rb_funcall(parent, rb_intern("private_methods"), 0);
    if (rb_funcall(ary, rb_intern("include?"), 1, method_sym) == Qtrue) {
        VALUE parent_run_func = rb_obj_method(parent, method_sym);
        state_machine->run_method = parent_run_func;
        rb_hash_aset(rb_ivar_get(state_machine_obj, rb_intern("@pool")), method_sym, parent_run_func);
    }
    return state_machine_obj;
}

static VALUE state_machine_get_name(VALUE self) {
    return rb_ivar_get(self, rb_intern("@state_name"));
}

static VALUE state_machine_get_old_name(VALUE self) {
    return rb_ivar_get(self, rb_intern("@old_state_name"));
}

void Init_statemachine(VALUE parent_module) {
    volatile VALUE class_state_machine= rb_define_class_under(parent_module, "StateMachine", rb_cObject);
    rb_define_alloc_func(class_state_machine, state_machine_alloc);
    rb_define_singleton_method(class_state_machine, "register", state_machine_register, -1);
    rb_define_private_method(class_state_machine, "initialize", state_machine_initialize, -1);
    rb_define_method(class_state_machine, "run", state_machine_run, -1);
    rb_define_method(class_state_machine, "add", state_machine_add, -1);
    rb_define_private_method(class_state_machine, "empty?", state_machine_is_empty, 0);
    rb_define_method(class_state_machine, "changed?", state_machine_is_change, 0);
    rb_define_method(class_state_machine, "change", state_machine_change, -1);
    rb_define_method(class_state_machine, "current", state_machine_get_name, 0);
    rb_define_method(class_state_machine, "old_current", state_machine_get_old_name, 0);
    rb_define_method(class_state_machine, "member", state_machine_get_member, 0);
    rb_define_method(class_state_machine, "member?", state_machine_is_member, 1);
    rb_define_method(class_state_machine, "pause", state_machine_pause, 0);
    rb_define_method(class_state_machine, "resume", state_machine_resume, 0);
    rb_define_method(class_state_machine, "call", state_machine_call, -1);

}
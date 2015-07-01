#include "sdl_udon_private.h"

static VALUE module_counter_new(int argc, VALUE argv[], VALUE self) {
    return rb_class_new_instance(argc, argv, rb_path2class("SDLUdon::Counter::Count"));
}

static VALUE module_counter_time(int argc, VALUE argv[], VALUE self) {
    return rb_class_new_instance(argc, argv, rb_path2class("SDLUdon::Counter::Time"));
}

static void count_mark(CounterData *counterData) {
    rb_gc_mark(counterData->nowCount);
    rb_gc_mark(counterData->startCount);
    rb_gc_mark(counterData->max);
    rb_gc_mark(counterData->min);
}

static VALUE count_alloc(VALUE klass) {
    CounterData* counterData = ALLOC(CounterData);
    counterData->nowCount = rb_int_new(0);
    counterData->startCount = rb_int_new(0);
    return Data_Wrap_Struct(klass, count_mark, NULL, counterData);
}

static VALUE count_initialize(int argc, VALUE argv[], VALUE self) {
    CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
    counterData->step = 1;
    VALUE init_count, step;
    rb_scan_args(argc, argv, "02", &step, &init_count);
    if (!NIL_P(init_count)) counterData->nowCount = counterData->startCount = init_count;
    if (!NIL_P(step)) counterData->step = NUM2INT(step);
    return Qnil;
}

static VALUE count_get_step(VALUE self) {
    CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
    return rb_int_new(counterData->step);
}

static VALUE count_set_step(VALUE self, VALUE step) {
    CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
    counterData->step = NUM2INT(step);
    return step;
}

static VALUE CheckCountLimit(VALUE num, VALUE self) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
   if (rb_funcall(counterData->min, rb_intern("<"), num) == Qtrue) num = counterData->min;
   if (rb_funcall(counterData->max, rb_intern(">"), num) == Qtrue) num = counterData->max;
   return num;
}

static void CountUpDown(int argc, VALUE argv[], VALUE self, const char *sign) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
    VALUE num;
    if (rb_scan_args(argc, argv, "01", &num) == 0) num = rb_int_new(counterData->step);
    VALUE count =rb_funcall(counterData->nowCount, rb_intern(sign), 1, num);
    counterData->nowCount = CheckCountLimit(count,self);
}

static VALUE count_up(int argc, VALUE argv[], VALUE self) {
    CountUpDown(argc, argv, self, "+");
    return self;
 }

static VALUE count_down(int argc, VALUE argv[], VALUE self) {
    CountUpDown(argc, argv, self, "-");
    return self;
 }

static VALUE count_current(VALUE self) {
    CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
    return counterData->nowCount;
}

static VALUE count_get_limit(VALUE self, VALUE ary_or_range) {
    CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
    return rb_range_new(counterData->min, counterData->max, 1);
}

static VALUE count_set_limit(VALUE self, VALUE ary_or_range) {
    CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
    if (rb_type_p(ary_or_range, T_ARRAY)) {
        counterData->min = RARRAY_PTR(ary_or_range)[0];
        counterData->max = RARRAY_PTR(ary_or_range)[1];
    } else {
        counterData->min = rb_funcall(ary_or_range, rb_intern("min"), 0);
        counterData->max = rb_funcall(ary_or_range, rb_intern("max"), 0);
   }
   return ary_or_range;
}

static VALUE count_get_min(VALUE self, VALUE ary_or_range) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
   return counterData->min;
}

static VALUE count_set_min(VALUE self, VALUE min) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
   counterData->min = min;
   return self;
}

static VALUE count_get_max(VALUE self, VALUE ary_or_range) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
   return counterData->max;
}

static VALUE count_set_max(VALUE self, VALUE max) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
   counterData->max = max;
   return self;
}

static VALUE count_reset(int argc, VALUE argv[], VALUE self) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
   VALUE init_count;rb_scan_args(argc, argv, "01", &init_count);
   if (!NIL_P(init_count)) counterData->startCount = init_count;
   counterData->nowCount = counterData->startCount;
   return self;
}

static VALUE time_initialize(int argc, VALUE argv[], VALUE self) {
    CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
    counterData->step = 0;
    counterData->startCount = rb_int_new(SDL_GetTicks());
    rb_call_super(argc, argv);
    return Qnil;
}

static VALUE time_current(VALUE self) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
   if (NUM2INT(counterData->nowCount) != 0) {
       return rb_funcall(counterData->nowCount, rb_intern("-"), 1, counterData->startCount);
   } else {
       return rb_funcall(rb_int_new(SDL_GetTicks()), rb_intern("-"), 1, counterData->startCount);
  }
}

static VALUE time_restart(VALUE self) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
   counterData->startCount = rb_int_new(SDL_GetTicks());
   counterData->nowCount = rb_int_new(0);
   return self;
}

static VALUE time_suspend(VALUE self) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
   counterData->nowCount = rb_int_new(SDL_GetTicks());
   return self;
}

static VALUE time_resume(VALUE self) {
   CounterData* counterData; Data_Get_Struct(self, CounterData, counterData);
   Uint32 suspendTime = SDL_GetTicks() - NUM2INT(counterData->nowCount);
   counterData->startCount = rb_funcall(counterData->startCount, rb_intern("+"), 1, rb_int_new(suspendTime));
   counterData->nowCount = rb_int_new(0);
   return self;
}

void Init_counter(VALUE parent_module) {
    VALUE module_counter = rb_define_module_under(parent_module, "Counter");
    rb_define_module_function(module_counter, "new", module_counter_new, -1);
    rb_define_module_function(module_counter, "num", module_counter_new, -1);
    rb_define_module_function(module_counter, "clock", module_counter_time, -1);
    VALUE class_count = rb_define_class_under(module_counter, "Count", rb_cObject);
    rb_define_alloc_func(class_count, count_alloc);
    rb_define_private_method(class_count, "initialize", count_initialize, -1);
    rb_define_method(class_count, "step=", count_set_step, 1);
    rb_define_method(class_count, "step", count_get_step, 0);
    rb_define_method(class_count, "up", count_up, -1);
    rb_define_method(class_count, "down", count_down, -1);
    rb_define_method(class_count, "current", count_current, 0);
    rb_define_method(class_count, "now", count_current, 0);
    rb_define_method(class_count, "limit=", count_set_limit, 1);
    rb_define_method(class_count, "limit", count_get_limit, 0);
    rb_define_method(class_count, "min", count_get_min, 0);
    rb_define_method(class_count, "min=", count_set_min, 1);
    rb_define_method(class_count, "max", count_get_max, 0);
    rb_define_method(class_count, "max=", count_set_max, 1);
    rb_define_method(class_count, "reset", count_reset, -1);
    VALUE class_time = rb_define_class_under(module_counter, "Clock", class_count);
    rb_define_private_method(class_time, "initialize", time_initialize, -1);
    rb_define_method(class_time, "current", time_current, 0);
    rb_define_method(class_time, "now", time_current, 0);
    rb_define_method(class_time, "restart", time_restart, 0);
    rb_define_method(class_time, "pause", time_suspend, 0);
    rb_define_method(class_time, "resume", time_resume, 0);
}
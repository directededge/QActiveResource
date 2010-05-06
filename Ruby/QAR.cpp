#include <ruby.h>
#include "ActiveResource.h"

extern "C"
{
    typedef VALUE(*ARGS)(...);

    static VALUE rb_mQAR;
    static VALUE rb_cQARResource;

    static void resource_mark(ActiveResource::Resource *resource)
    {

    }

    static void resource_free(ActiveResource::Resource *resource)
    {
        delete resource;
    }

    static VALUE resource_allocate(VALUE klass)
    {
        ActiveResource::Resource *resource = new ActiveResource::Resource;
        return Data_Wrap_Struct(klass, resource_mark, resource_free, resource);
    }

    static VALUE resource_find(VALUE self)
    {
        printf("find!\n");
        return 0;
    }

    void Init_QAR(void)
    {
        rb_mQAR = rb_define_module("QAR");
        rb_cQARResource = rb_define_class_under(rb_mQAR, "Resource", rb_cObject);
        rb_define_alloc_func(rb_cQARResource, resource_allocate);
        rb_define_method(rb_cQARResource, "find", (ARGS) resource_find, 0);
    }
}

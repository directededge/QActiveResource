#include <ruby.h>
#include "ActiveResource.h"

/*
 * Used by Ruby.
 */

static VALUE rb_mQAR;
static VALUE rb_cQARResource;

extern "C"
{
    static void resource_mark(ActiveResource::Resource *resource)
    {

    }

    static void resource_free(ActiveResource::Resource *resource)
    {
        delete resource;
    }

    static VALUE resource_allocate(VALUE klass)
    {
        ActiveResource::Resource *resource = 0;
        return Data_Wrap_Struct(klass, resource_mark, resource_free, resource);
    }

    void Init_QAR(void)
    {
        rb_mQAR = rb_define_module("QAR");
        rb_cQARResource = rb_define_class_under(rb_mQAR, "Resource", rb_cObject);
        rb_define_alloc_func(rb_cQARResource, resource_allocate);
    }

}

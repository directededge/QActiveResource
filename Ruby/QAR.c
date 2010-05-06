#include <ruby.h>

typedef struct
{

} QARResource;

static QARResource *qar_resource_new(void)
{
    QARResource *resource = malloc(sizeof(QARResource));
    return resource;
}

static void qar_resource_delete(QARResource *resource)
{
    free(resource);
}

/*
 * Used by Ruby.
 */

static VALUE rb_mQAR;
static VALUE rb_cQARResource;

static void resource_mark(QARResource *resource)
{
    
}

static void resource_free(QARResource *resource)
{
    qar_resource_delete(resource);
}

static VALUE resource_allocate(VALUE klass)
{
    QARResource *resource = qar_resource_new();
    return Data_Wrap_Struct(klass, resource_mark, resource_free, resource);
}

void Init_QAR(void)
{
    rb_mQAR = rb_define_module("QAR");
    rb_cQARResource = rb_define_class_under(rb_mQAR, "Resource", rb_cObject);
    rb_define_alloc_func(rb_cQARResource, resource_allocate);
}

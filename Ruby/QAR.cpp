/*
 * Copyright (C) 2010, Directed Edge, Inc. | Licensed under the MPL and LGPL
 */

#include <QActiveResource.h>
#include <ruby.h>

extern "C"
{
    typedef VALUE(*ARGS)(...);

    static VALUE rb_mQAR;
    static VALUE rb_cQARResource;

    static void resource_mark(QActiveResource::Resource *resource)
    {

    }

    static void resource_free(QActiveResource::Resource *resource)
    {
        delete resource;
    }

    static VALUE resource_allocate(VALUE klass)
    {
        QActiveResource::Resource *resource = new QActiveResource::Resource;
        return Data_Wrap_Struct(klass, resource_mark, resource_free, resource);
    }

    static VALUE resource_initialize(VALUE self, VALUE base, VALUE resource)
    {
        QActiveResource::Resource *r = 0;
        Data_Get_Struct(self, class QActiveResource::Resource, r);
        r->setBase(QString::fromUtf8(rb_string_value_ptr(&base)));
        r->setResource(QString::fromUtf8(rb_string_value_ptr(&resource)));
    }

    static VALUE to_value(const QVariant &v)
    {
        switch(v.type())
        {
        case QVariant::Hash:
        {
            VALUE value = rb_hash_new();

            QHash<QString, QVariant> hash = v.toHash();

            for(QHash<QString, QVariant>::ConstIterator it = hash.begin(); it != hash.end(); ++it)
            {
                rb_hash_aset(value, ID2SYM(rb_intern(it.key().toUtf8())), to_value(it.value()));
            }

            return value;
        }
        case QVariant::List:
        {
            VALUE value = rb_ary_new();

            foreach(QVariant element, v.toList())
            {
                rb_ary_push(value, to_value(element));
            }

            return value;
        }
        case QVariant::Bool:
            return v.toBool() ? Qtrue : Qfalse;
        case QVariant::Int:
            return rb_int_new(v.toInt());
        case QVariant::Double:
            return rb_float_new(v.toDouble());
        default:
            return rb_str_new2(v.toString().toUtf8());
        }
    }

    static VALUE resource_find(VALUE self)
    {
        QActiveResource::Resource *resource = 0;
        Data_Get_Struct(self, class QActiveResource::Resource, resource);

        QActiveResource::RecordList records = resource->find();

        VALUE array = rb_ary_new2(records.length());

        for(int i = 0; i < records.length(); i++)
        {
            rb_ary_store(array, i, to_value(records[i]));
        }

        return array;
    }

    void Init_QAR(void)
    {
        rb_mQAR = rb_define_module("QAR");
        rb_cQARResource = rb_define_class_under(rb_mQAR, "Resource", rb_cObject);
        rb_define_alloc_func(rb_cQARResource, resource_allocate);
        rb_define_method(rb_cQARResource, "initialize", (ARGS) resource_initialize, 2);
        rb_define_method(rb_cQARResource, "find", (ARGS) resource_find, 0);
    }
}

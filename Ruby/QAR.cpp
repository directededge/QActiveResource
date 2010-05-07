
/*
 * Copyright (C) 2010, Directed Edge, Inc. | Licensed under the MPL and LGPL
 */

#include <QActiveResource.h>
#include <QDateTime>
#include <ruby.h>
#include <QDebug>

extern "C"
{
    typedef VALUE(*ARGS)(...);
    typedef int(*ITERATOR)(...);

    static VALUE rb_mQAR;
    static VALUE rb_cQARHash;
    static VALUE rb_cQARParamList;
    static VALUE rb_cQARResource;

    /*
     * helper functions
     */

    static QString to_s(VALUE value)
    {
        static const ID id = rb_intern("to_s");
        VALUE s = rb_funcall(value, id, 0);
        return QString::fromUtf8(StringValuePtr(s));
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
        case QVariant::Invalid:
            return Qnil;
        case QVariant::Bool:
            return v.toBool() ? Qtrue : Qfalse;
        case QVariant::Int:
            return rb_int_new(v.toInt());
        case QVariant::Double:
            return rb_float_new(v.toDouble());
        case QVariant::DateTime:
        {
            static const ID at = rb_intern("at");
            return rb_funcall(rb_cTime, at, 1, rb_int_new(v.toDateTime().toTime_t()));
        }
        default:
            return rb_str_new2(v.toString().toUtf8());
        }
    }

    /*
     * Hash
     */

    static VALUE hash_method_missing(VALUE self, VALUE method)
    {
        VALUE value = rb_hash_aref(self, method);


        if(value == Qnil)
        {
            static const ID symbol = rb_intern("method_missing");
            return rb_funcall(rb_cHash, symbol, 1, method);
        }

        return value;
    }

    /*
     * Resource
     */

    static void resource_mark(QActiveResource::Resource *) {}
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
        Data_Get_Struct(self, QActiveResource::Resource, r);
        r->setBase(QString::fromUtf8(rb_string_value_ptr(&base)));
        r->setResource(QString::fromUtf8(rb_string_value_ptr(&resource)));
    }

    /*
     * ParamList
     */

    static VALUE param_list_mark(QActiveResource::ParamList *) {}
    static VALUE param_list_free(QActiveResource::ParamList *params)
    {
        delete params;
    }

    static VALUE param_list_allocate(VALUE klass)
    {
        QActiveResource::ParamList *params = new QActiveResource::ParamList();
        return Data_Wrap_Struct(klass, param_list_mark, param_list_free, params);
    }

    static int params_hash_iterator(VALUE key, VALUE value, VALUE params)
    {
        QActiveResource::ParamList *params_pointer;
        Data_Get_Struct(params, QActiveResource::ParamList, params_pointer);
        params_pointer->append(QActiveResource::Param(to_s(key), to_s(value)));
    }

    static VALUE resource_find(int argc, VALUE *argv, VALUE self)
    {
        QActiveResource::Resource *resource = 0;
        Data_Get_Struct(self, class QActiveResource::Resource, resource);

        VALUE params = param_list_allocate(rb_cData);

        if(argc >= 2 && TYPE(argv[1]) == T_HASH)
        {
            static const VALUE params_symbol = ID2SYM(rb_intern("params"));
            VALUE params_hash = rb_hash_aref(argv[1], params_symbol);

            if(params_hash != Qnil)
            {
                rb_hash_foreach(params_hash, (ITERATOR) params_hash_iterator, params);
            }

            static const VALUE follow_redirects_symbol = ID2SYM(rb_intern("follow_redirects"));
            VALUE follow_redirects = rb_hash_aref(argv[1], follow_redirects_symbol);
            resource->setFollowRedirects(follow_redirects == Qtrue);
        }

        QActiveResource::ParamList *params_pointer;
        Data_Get_Struct(params, QActiveResource::ParamList, params_pointer);

        QString from;

        if(argc >= 1)
        {
            static const ID one = rb_intern("one");
            static const ID first = rb_intern("first");
            static const ID last = rb_intern("last");
            static const ID all = rb_intern("all");

            ID current = SYM2ID(argv[0]);

            if(current == one)
            {
                return to_value(resource->find(QActiveResource::FindOne, from, *params_pointer));
            }
            else if(current == first)
            {
                return to_value(resource->find(QActiveResource::FindFirst, from, *params_pointer));
            }
            else if(current == last)
            {
                return to_value(resource->find(QActiveResource::FindLast, from, *params_pointer));
            }
            else if(current != all)
            {
                return to_value(resource->find(to_s(argv[0])));
            }
        }

        QActiveResource::RecordList records =
            resource->find(QActiveResource::FindAll, from, *params_pointer);

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

        rb_cQARHash = rb_define_class_under(rb_mQAR, "Hash", rb_cHash);
        rb_define_method(rb_cQARHash, "method_missing", (ARGS) hash_method_missing, 1);

        rb_cQARParamList = rb_define_class_under(rb_mQAR, "ParamList", rb_cObject);
        rb_define_alloc_func(rb_cQARParamList, param_list_allocate);

        rb_cQARResource = rb_define_class_under(rb_mQAR, "Resource", rb_cObject);
        rb_define_alloc_func(rb_cQARResource, resource_allocate);
        rb_define_method(rb_cQARResource, "initialize", (ARGS) resource_initialize, 2);
        rb_define_method(rb_cQARResource, "find", (ARGS) resource_find, -1);
    }
}

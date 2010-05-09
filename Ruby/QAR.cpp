
/*
 * Copyright (C) 2010, Directed Edge, Inc. | Licensed under the MPL and LGPL
 */

#include <QActiveResource.h>
#include <QDateTime>
#include <QDebug>
#include <ruby.h>

typedef VALUE(*ARGS)(...);
typedef int(*ITERATOR)(...);

/*
 * Modules
 */

static VALUE rb_mQAR;
static VALUE rb_mActiveResource;

/*
 * Classes
 */

static VALUE rb_cActiveResourceBase;
static VALUE rb_cQARParamList;
static VALUE rb_cQARResource;

/*
 * Symbols
 */

static const ID _all = rb_intern("all");
static const ID _allocate = rb_intern("allocate");
static const ID _at = rb_intern("at");
static const ID _collection_name = rb_intern("collection_name");
static const ID _element_name = rb_intern("element_name");
static const ID _first = rb_intern("first");
static const ID _follow_redirects = rb_intern("follow_redirects");
static const ID _new = rb_intern("new");
static const ID _one = rb_intern("one");
static const ID _params = rb_intern("params");
static const ID _site = rb_intern("site");
static const ID _to_s = rb_intern("to_s");
static const ID _last = rb_intern("last");

/*
 * Instance variable symbols
 */

static const ID __attributes = rb_intern("@attributes");
static const ID __prefix_options = rb_intern("@prefix_options");
static const ID __qar_resource = rb_intern("@qar_resource");

static QString to_s(VALUE value)
{
    VALUE s = rb_funcall(value, _to_s, 0);
    return QString::fromUtf8(StringValuePtr(s));
}

static VALUE to_value(const QVariant &v, VALUE base, bool isChild = false)
{
    switch(v.type())
    {
    case QVariant::Hash:
    {
        QActiveResource::Record record = v;

        VALUE attributes = rb_hash_new();

        if(record.isEmpty())
        {
            return attributes;
        }

        VALUE klass = base;

        if(isChild)
        {
            klass = rb_define_class_under(base, record.className().toUtf8(), rb_cActiveResourceBase);
        }

        for(QActiveResource::Record::ConstIterator it = record.begin(); it != record.end(); ++it)
        {
            VALUE key = rb_str_new2(it.key().toUtf8());
            VALUE value = to_value(it.value(), base, true);
            rb_hash_aset(attributes, key, value);
        }

        VALUE value = rb_funcall(klass, _allocate, 0);
        rb_ivar_set(value, __attributes, attributes);
        rb_ivar_set(value, __prefix_options, rb_hash_new());
        return value;
    }
    case QVariant::List:
    {
        VALUE value = rb_ary_new();

        foreach(QVariant element, v.toList())
        {
            rb_ary_push(value, to_value(element, base, isChild));
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
        return rb_funcall(rb_cTime, _at, 1, rb_int_new(v.toDateTime().toTime_t()));
    }
    default:
        return rb_str_new2(v.toString().toUtf8());
    }
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

/*
 * QAR
 */

static VALUE qar_find(int argc, VALUE *argv, VALUE self)
{
    VALUE member = rb_ivar_get(self, __qar_resource);
    QActiveResource::Resource *resource = 0;

    if(member == Qnil)
    {
        member = rb_funcall(rb_cQARResource, _new, 0);
        rb_ivar_set(self, __qar_resource, member);
        Data_Get_Struct(member, QActiveResource::Resource, resource);
        resource->setBase(to_s(rb_funcall(self, _site, 0)));
    }
    else
    {
        Data_Get_Struct(member, QActiveResource::Resource, resource);
    }

    VALUE params = param_list_allocate(rb_cData);

    if(argc >= 2 && TYPE(argv[1]) == T_HASH)
    {
        VALUE params_hash = rb_hash_aref(argv[1], ID2SYM(_params));

        if(params_hash != Qnil)
        {
            rb_hash_foreach(params_hash, (ITERATOR) params_hash_iterator, params);
        }

        VALUE follow_redirects = rb_hash_aref(argv[1], ID2SYM(_follow_redirects));
        resource->setFollowRedirects(follow_redirects == Qtrue);
    }

    QActiveResource::ParamList *params_pointer;
    Data_Get_Struct(params, QActiveResource::ParamList, params_pointer);

    QString from;
    resource->setResource(to_s(rb_funcall(self, _collection_name, 0)));

    if(argc >= 1)
    {
        ID current = SYM2ID(argv[0]);

        if(current == _one)
        {
            resource->setResource(to_s(rb_funcall(self, _element_name, 0)));
            return to_value(resource->find(QActiveResource::FindOne, from, *params_pointer), self);
        }
        else if(current == _first)
        {
            return to_value(resource->find(QActiveResource::FindFirst, from, *params_pointer), self);
        }
        else if(current == _last)
        {
            return to_value(resource->find(QActiveResource::FindLast, from, *params_pointer), self);
        }
        else if(current != _all)
        {
            resource->setResource(to_s(rb_funcall(self, _element_name, 0)));
            return to_value(resource->find(to_s(argv[0])), self);
        }
    }

    QActiveResource::RecordList records =
        resource->find(QActiveResource::FindAll, from, *params_pointer);

    VALUE array = rb_ary_new2(records.length());

    for(int i = 0; i < records.length(); i++)
    {
        rb_ary_store(array, i, to_value(records[i], self));
    }

    return array;
}

extern "C"
{
    void Init_QAR(void)
    {
        rb_mActiveResource = rb_define_module("ActiveResource");
        rb_cActiveResourceBase = rb_define_class_under(rb_mActiveResource, "Base", rb_cObject);

        rb_mQAR = rb_define_module("QAR");

        rb_cQARParamList = rb_define_class_under(rb_mQAR, "ParamList", rb_cObject);
        rb_define_alloc_func(rb_cQARParamList, param_list_allocate);

        rb_cQARResource = rb_define_class_under(rb_mQAR, "Resource", rb_cObject);
        rb_define_alloc_func(rb_cQARResource, resource_allocate);

        rb_define_method(rb_mQAR, "find", (ARGS) qar_find, -1);
    }
}

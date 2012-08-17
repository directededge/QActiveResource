
/*
 * Copyright (C) 2010, Directed Edge, Inc. | Licensed under the MPL and LGPL
 */

#include "QActiveResource.h"
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
static VALUE rb_cQARResponse;

/*
 * Exceptions
 */

static VALUE rb_eActiveResourceConnectionError;
static VALUE rb_eActiveResourceTimeoutError;
static VALUE rb_eActiveResourceSSLError;
static VALUE rb_eActiveResourceRedirection;
static VALUE rb_eActiveResourceClientError;
static VALUE rb_eActiveResourceBadRequest;
static VALUE rb_eActiveResourceUnauthorizedAccess;
static VALUE rb_eActiveResourceForbiddenAccess;
static VALUE rb_eActiveResourceResourceNotFound;
static VALUE rb_eActiveResourceMethodNotAllowed;
static VALUE rb_eActiveResourceResourceConflict;
static VALUE rb_eActiveResourceResourceGone;
static VALUE rb_eActiveResourceServerError;

/*
 * Symbols
 */

static const ID _all = rb_intern("all");
static const ID _allocate = rb_intern("allocate");
static const ID _at = rb_intern("at");
static const ID _code = rb_intern("code");
static const ID _body = rb_intern("body");
static const ID _collection_name = rb_intern("collection_name");
static const ID _element_name = rb_intern("element_name");
static const ID _extend = rb_intern("extend");
static const ID _first = rb_intern("first");
static const ID _from = rb_intern("from");
static const ID _headers = rb_intern("headers");
static const ID _new = rb_intern("new");
static const ID _one = rb_intern("one");
static const ID _params = rb_intern("params");
static const ID _raise = rb_intern("raise");
static const ID _site = rb_intern("site");
static const ID _to_s = rb_intern("to_s");
static const ID _last = rb_intern("last");

/*
 * Instance variable symbols
 */

static const ID __attributes = rb_intern("@attributes");
static const ID __code = rb_intern("@code");
static const ID __body = rb_intern("@body");
static const ID __headers = rb_intern("@headers");
static const ID __prefix_options = rb_intern("@prefix_options");
static const ID __response = rb_intern("@response");
static const ID __message = rb_intern("@message");

/*
 * Class variable symbols
 */

static const ID ___qar_resource = rb_intern("@@qar_resource");

static QString to_s(VALUE value)
{
    VALUE s = rb_funcall(value, _to_s, 0);
    return QString::fromUtf8(StringValuePtr(s));
}

static VALUE to_value(const QString &s)
{
    return rb_str_new2(s.toUtf8());
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
            QString name = record.className();
            klass = rb_define_class_under(base, name.toUtf8(), rb_cActiveResourceBase);
        }

        for(QActiveResource::Record::ConstIterator it = record.begin(); it != record.end(); ++it)
        {
            VALUE key = to_value(it.key());
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
        return to_value(v.toString());
    }
}

static VALUE to_value(const QHash<QString, QString> &hash)
{
    VALUE values = rb_hash_new();

    for(QHash<QString, QString>::ConstIterator it = hash.begin(); it != hash.end(); ++it)
    {
        rb_hash_aset(values, to_value(it.key()), to_value(it.value()));
    }

    return values;
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

static int params_hash_iterator_append_subhash(VALUE key, VALUE value, VALUE subHash)
{
    VALUE subhashKey = rb_hash_aref(subHash, rb_str_new2("key"));
    VALUE params = rb_hash_aref(subHash, rb_str_new2("params"));
    QActiveResource::ParamList *params_pointer;
    Data_Get_Struct(params, QActiveResource::ParamList, params_pointer);
    QString subKey = to_s(subhashKey) + "[" + to_s(key) + "]";

    int tv = TYPE(value);

    if(tv == T_STRING || tv == T_FLOAT || tv == T_FIXNUM
       || tv == T_BIGNUM || tv == T_SYMBOL)
    {
        params_pointer->append(QActiveResource::Param(subKey, to_s(value)));
    }
    else if(tv == T_HASH)
    {
        qCritical() << "QActiveResource: Nesting level to deep.";
    }
    else
    {
        params_pointer->append(QActiveResource::Param(subKey, to_s(value)));
        qCritical() << "QActiveResource: Value type of nested key" << to_s(key) << "not supported.";
    }
}

static int params_hash_iterator(VALUE key, VALUE value, VALUE params)
{
    QActiveResource::ParamList *params_pointer;
    Data_Get_Struct(params, QActiveResource::ParamList, params_pointer);

    int tv = TYPE(value);

    if(tv == T_HASH)
    {
        VALUE sub_hash = rb_hash_new();
        rb_hash_aset(sub_hash, rb_str_new2("key"), key);
        rb_hash_aset(sub_hash, rb_str_new2("params"), params);
        rb_hash_foreach(value, (ITERATOR) params_hash_iterator_append_subhash, sub_hash);
    }
    else if(tv == T_STRING || tv == T_FLOAT || tv == T_FIXNUM
            || tv == T_BIGNUM || tv == T_SYMBOL)
    {
        params_pointer->append(QActiveResource::Param(to_s(key), to_s(value)));
    }
    else
    {
        params_pointer->append(QActiveResource::Param(to_s(key), to_s(value)));
        qCritical() << "QActiveResource: Value type of key" << to_s(key) << "not supported.";
    }
}

/*
 * Response
 */

static VALUE response_initialize(VALUE self, VALUE code, VALUE headers, VALUE body)
{
    rb_ivar_set(self, __code, code);
    rb_ivar_set(self, __headers, headers);
    rb_ivar_set(self, __body, body);
}

static VALUE response_index_operator(VALUE self, VALUE index)
{
    return rb_hash_aref(rb_ivar_get(self, __headers), index);
}

/*
 * QAR
 */

static QActiveResource::Resource *get_resource(VALUE self)
{
    VALUE member = rb_cvar_get(self, ___qar_resource);
    QActiveResource::Resource *resource = 0;
    Data_Get_Struct(member, QActiveResource::Resource, resource);
    return resource;
}

static VALUE qar_find(int argc, VALUE *argv, VALUE self)
{
    QActiveResource::Resource *resource = get_resource(self);
    resource->setBase(to_s(rb_funcall(self, _site, 0)));

    QString from;
    VALUE params = param_list_allocate(rb_cData);

    if(argc >= 2 && TYPE(argv[1]) == T_HASH)
    {
        VALUE params_hash = rb_hash_aref(argv[1], ID2SYM(_params));

        if(params_hash != Qnil && TYPE(params_hash) == T_HASH)
        {
            rb_hash_foreach(params_hash, (ITERATOR) params_hash_iterator, params);
        }

        from = to_s(rb_hash_aref(argv[1], ID2SYM(_from)));
    }

    QActiveResource::ParamList *params_pointer;
    Data_Get_Struct(params, QActiveResource::ParamList, params_pointer);
    resource->setResource(to_s(rb_funcall(self, _collection_name, 0)));

    try
    {
        if(argc >= 1)
        {
            ID current = SYM2ID(argv[0]);

            if(current == _one)
            {
                resource->setResource(to_s(rb_funcall(self, _element_name, 0)));
                QVariant v = resource->find(QActiveResource::FindOne, from, *params_pointer);
                return to_value(v, self);
            }
            else if(current == _first)
            {
                QVariant v = resource->find(QActiveResource::FindFirst, from, *params_pointer);
                return to_value(v, self);
            }
            else if(current == _last)
            {
                QVariant v = resource->find(QActiveResource::FindLast, from, *params_pointer);
                return to_value(v, self);
            }
            else if(current != _all)
            {
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
    catch(QActiveResource::Exception ex)
    {
        VALUE code = rb_int_new(ex.response().code());
        VALUE response = rb_funcall(rb_cQARResponse, _new, 3,
                                    code,
                                    to_value(ex.response().headers()),
                                    rb_str_new2(ex.response().data()));

        #define AR_TEST_EXCEPTION(name)                                 \
            if(ex.type() == QActiveResource::Exception::name)           \
            {                                                           \
                VALUE e = rb_funcall(                                   \
                    rb_eActiveResource##name, _allocate, 0);            \
                rb_ivar_set(e, __code, code);                           \
                rb_ivar_set(e, __response, response);                   \
                rb_ivar_set(e, __message, to_value(ex.message()));      \
                rb_exc_raise(e);                                        \
            }

        AR_TEST_EXCEPTION(ConnectionError);
        AR_TEST_EXCEPTION(TimeoutError);
        AR_TEST_EXCEPTION(SSLError);
        AR_TEST_EXCEPTION(Redirection);
        AR_TEST_EXCEPTION(ClientError);
        AR_TEST_EXCEPTION(BadRequest);
        AR_TEST_EXCEPTION(UnauthorizedAccess);
        AR_TEST_EXCEPTION(ForbiddenAccess);
        AR_TEST_EXCEPTION(ResourceNotFound);
        AR_TEST_EXCEPTION(MethodNotAllowed);
        AR_TEST_EXCEPTION(ResourceConflict);
        AR_TEST_EXCEPTION(ResourceGone);
        AR_TEST_EXCEPTION(ServerError);

        return Qnil;
    }
}

static VALUE set_follow_redirects(VALUE self, VALUE follow)
{
    QActiveResource::Resource *resource = get_resource(self);
    resource->setFollowRedirects(follow == Qtrue);
    return follow;
}

static VALUE qar_extended(VALUE self, VALUE base)
{
    VALUE resource = rb_funcall(rb_cQARResource, _new, 0);
    rb_cvar_set(base, ___qar_resource, resource, false);
    return Qnil;
}

extern "C"
{
    void Init_QAR(void)
    {
        #define DEFINE_CLASS(module, name) \
            rb_c##module##name = rb_define_class_under(rb_m##module, #name, rb_cObject)

        rb_mActiveResource = rb_define_module("ActiveResource");

        DEFINE_CLASS(ActiveResource, Base);

        #define AR_DEFINE_EXCEPTION(name, base) \
            rb_eActiveResource##name = rb_define_class_under(rb_mActiveResource, #name, rb_e##base)

        AR_DEFINE_EXCEPTION(ConnectionError, StandardError);
        AR_DEFINE_EXCEPTION(TimeoutError, ActiveResourceConnectionError);
        AR_DEFINE_EXCEPTION(SSLError, ActiveResourceConnectionError);
        AR_DEFINE_EXCEPTION(Redirection, ActiveResourceConnectionError);
        AR_DEFINE_EXCEPTION(ClientError, ActiveResourceConnectionError);
        AR_DEFINE_EXCEPTION(BadRequest, ActiveResourceClientError);
        AR_DEFINE_EXCEPTION(UnauthorizedAccess, ActiveResourceClientError);
        AR_DEFINE_EXCEPTION(ForbiddenAccess, ActiveResourceClientError);
        AR_DEFINE_EXCEPTION(ResourceNotFound, ActiveResourceClientError);
        AR_DEFINE_EXCEPTION(MethodNotAllowed, ActiveResourceClientError);
        AR_DEFINE_EXCEPTION(ResourceConflict, ActiveResourceClientError);
        AR_DEFINE_EXCEPTION(ResourceGone, ActiveResourceClientError);
        AR_DEFINE_EXCEPTION(ServerError, ActiveResourceConnectionError);

        rb_mQAR = rb_define_module("QAR");

        DEFINE_CLASS(QAR, Response);
        rb_define_method(rb_cQARResponse, "initialize", (ARGS) response_initialize, 3);
        rb_define_method(rb_cQARResponse, "[]", (ARGS) response_index_operator, 1);
        rb_attr(rb_cQARResponse, _code, 1, 0, Qfalse);
        rb_attr(rb_cQARResponse, _headers, 1, 0, Qfalse);
        rb_attr(rb_cQARResponse, _body, 1, 0, Qfalse);

        DEFINE_CLASS(QAR, ParamList);
        rb_define_alloc_func(rb_cQARResponse, resource_allocate);

        rb_define_alloc_func(rb_cQARParamList, param_list_allocate);

        rb_cQARResource = rb_define_class_under(rb_mQAR, "Resource", rb_cObject);
        rb_define_alloc_func(rb_cQARResource, resource_allocate);

        rb_define_method(rb_mQAR, "find", (ARGS) qar_find, -1);
        rb_define_method(rb_mQAR, "follow_redirects=", (ARGS) set_follow_redirects, 1);
        rb_define_singleton_method(rb_mQAR, "extended", (ARGS) qar_extended, 1);
    }
}

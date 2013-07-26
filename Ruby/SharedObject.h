
/*
 * Copyright (C) 2010-2013, Directed Edge, Inc. | Licensed under the MPL and LGPL
 */

#include <ruby.h>
#include <QDebug>
#include <QList>

namespace SharedObject
{
    class Scope
    {
    public:
        Scope()
        {
            m_objects = rb_ary_new();
            rb_gc_register_address(&m_objects);
        }

        ~Scope()
        {
            rb_gc_unregister_address(&m_objects);
        }

        void registerObject(VALUE obj)
        {
            rb_ary_push(m_objects, obj);
        }

    private:
        VALUE m_objects;
    };

    template <class T>
    class Wrapper
    {
    public:
        Wrapper(Scope &scope)
        {
            T *object = new T();
            m_value = Data_Wrap_Struct(rb_cData, 0, &free_obj, object);
            scope.registerObject(m_value);
        }

        Wrapper(VALUE val)
        {
            m_value = val;
        }

        T *ptr()
        {
            T *obj = 0;
            Data_Get_Struct(m_value, T, obj);

            return obj;
        }

        T * operator-> ()
        {
            return ptr();
        }

        VALUE value()
        {
            return m_value;
        }

        static void free_obj(T *object)
        {
            delete object;
        }

    private:
        VALUE m_value;
    };
}

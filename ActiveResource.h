/*
 * Copyright (C) 2010, Directed Edge, Inc.
 */

#include <QUrl>
#include <QSharedData>
#include <QHash>
#include <QVariant>

namespace ActiveResource
{
    class Record
    {
    public:
        Record();
        QHash<QString, QVariant> fields() const;
        QVariant operator[](const QString &key) const;
    private:
        struct Data;
        QSharedDataPointer<Data> d;
    };

    typedef QList<Record> RecordList;

    enum FindStyle
    {
        FindOne,
        FindFirst,
        FindLast,
        FindAll
    };

    class Param
    {
    public:
        Param();
        Param(const QString &key, const QString &value);
        bool isNull() const;
        QString key() const;
        QString value() const;
    private:
        struct Data;
        QSharedDataPointer<Data> d;
    };

    class Base
    {
    public:
        RecordList find(FindStyle style = FindAll, const QString &from = QString(),
                        const Param &first = Param(), const Param &second = Param(),
                        const Param &third = Param(), const Param &fourth = Param());
    protected:
        Base(const QUrl &base);
    private:
        struct Data;
        QSharedDataPointer<Data> d;
    };
}

/*
 * Copyright (C) 2010, Directed Edge, Inc.
 */

#include <QUrl>
#include <QSharedData>
#include <QHash>
#include <QVariant>

namespace ActiveResource
{
    typedef QHash<QString, QVariant> Record;
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
        struct Data : public QSharedData
        {
            Data(const QString &k, const QString &v) : key(k), value(v) {}
            QString key;
            QString value;
        };

        QSharedDataPointer<Data> d;
    };

    class Base
    {
    public:
        RecordList find(FindStyle style, const QString &from, const QList<Param> &params);
        RecordList find(FindStyle style = FindAll, const QString &from = QString(),
                        const Param &first = Param(), const Param &second = Param(),
                        const Param &third = Param(), const Param &fourth = Param());
    protected:
        Base(const QUrl &base);
    private:
        struct Data : public QSharedData
        {
            Data(const QUrl &b) : base(b) {}
            QUrl base;
        };

        QSharedDataPointer<Data> d;
    };
}

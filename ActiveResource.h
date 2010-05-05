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

    enum FindSingle
    {
        FindOne,
        FindFirst,
        FindLast,
    };

    enum FindMulti
    {
        FindAll
    };

    class Resource
    {
    public:
        Resource(const QUrl &base, const QString &resource = QString());

        /*!
         * \return The record with the given text / numeric ID.
         */
        Record find(QVariant id) const;

        /*!
         * Finds a record using \a style.  \a from specifies a specific resource that
         * should be used that is below the base / resource and \a params is a list
         * of parameters that should be appended to the query string.
         */
        Record find(FindSingle style, const QString &from, const QList<Param> &params) const;

        /*!
         * Finds a record using \a style.  \a from specifies a specific resource that
         * should be used that is below the base / resource and \a params is a list
         * of parameters that should be appended to the query string.
         */
        RecordList find(FindMulti style, const QString &from, const QList<Param> &params) const;

        /*!
         * Convenience overload of the above that lets the parameters be specified
         * directly in the function call.
         */
        Record find(FindSingle style, const QString &from = QString(),
                    const Param &first = Param(), const Param &second = Param(),
                    const Param &third = Param(), const Param &fourth = Param()) const;
        /*!
         * Convenience overload of the above that lets the parameters be specified
         * directly in the function call.
         */
        RecordList find(FindMulti style = FindAll, const QString &from = QString(),
                        const Param &first = Param(), const Param &second = Param(),
                        const Param &third = Param(), const Param &fourth = Param()) const;
    private:
        RecordList find(QUrl url) const;
        QUrl url() const;

        struct Data : public QSharedData
        {
            Data(const QUrl &b, const QString &r) : base(b), resource(r), url(base)
            {
                url.setPath(url.path() + "/" + resource);
            }
            QUrl base;
            QString resource;
            QUrl url;
        };

        QSharedDataPointer<Data> d;
    };
}

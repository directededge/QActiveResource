/*
 * Copyright (C) 2010, Directed Edge, Inc. | Licensed under the MPL and LGPL
 */

#include <QUrl>
#include <QSharedData>
#include <QHash>
#include <QVariant>

namespace QActiveResource
{
    typedef QHash<QString, QVariant> Record;
    typedef QList<Record> RecordList;

    /*!
     * Used as parameters to Resource::find() to specify additional constraints.
     * These correspond to the options passed in the options hash in the Ruby
     * equivalent.
     */

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
            Data(const QString &key, const QString &value);
            QString key;
            QString value;
        };

        QSharedDataPointer<Data> d;
    };

    typedef QList<Param> ParamList;

    /*!
     * Used with Resource::find() to specify that only one record should be
     * returned.
     */

    enum FindSingle
    {
        FindOne,
        FindFirst,
        FindLast,
    };

    /*!
     * Used with Resource::find() to specify that multiple records should be
     * returned.
     */

    enum FindMulti
    {
        FindAll
    };

    /*!
     * Represents an ActiveResource resource.  The semantics are similar to Ruby's
     * ActiveResource::Base, however, instead of subclassing the class, the base
     * and resource name are provided in the constructor.
     */

    class Resource
    {
    public:
        /*!
         * Instantiates a resource starting at \a base using \a resource.
         * Authentication info may be included in the URL.
         *
         * For example:
         *
         *   - Base: "http://www.someshop.com/"
         *   - Resource: "products"
         */
        Resource(const QUrl &base = QUrl(), const QString &resource = QString());

        /*!
         * Sets the base URL of the resource to \a base.
         */
        void setBase(const QUrl &base);

        /*!
         * Sets the resource (i.e. "customers") of the object.
         */
        void setResource(const QString &resource);

        /*!
         * \return The record with the given text / numeric ID.
         */
        Record find(const QVariant &id) const;

        /*!
         * Finds a record using \a style.  \a from specifies a specific resource that
         * should be used that is below the base / resource and \a params is a list
         * of parameters that should be appended to the query string.
         */
        Record find(FindSingle style, const QString &from, const ParamList &params) const;

        /*!
         * Finds a record using \a style.  \a from specifies a specific resource that
         * should be used that is below the base / resource and \a params is a list
         * of parameters that should be appended to the query string.
         */
        RecordList find(FindMulti style, const QString &from, const ParamList &params) const;

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

        /*!
         * Enables following redirects if \a follow is true.
         *
         * \warning If authentication information is included, this will be
         * passed on to the sites where the redirection is sent.
         */
        void setFollowRedirects(bool follow);

    private:
        struct Data : public QSharedData
        {
            Data(const QUrl &base, const QString &resource);
            void setUrl();
            QUrl base;
            QString resource;
            QUrl url;
            bool followRedirects;
        };

        QSharedDataPointer<Data> d;
    };
}

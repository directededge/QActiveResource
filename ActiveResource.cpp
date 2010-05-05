/*
 * Copyright (C) 2010, Directed Edge, Inc.
 */

#include "ActiveResource.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>

using namespace ActiveResource;

struct Record::Data : public QSharedData
{
    QHash<QString, QVariant> fields;
};

Record::Record() :
    d(new Data)
{

}

struct Param::Data : public QSharedData
{
    Data(const QString &k, const QString &v) : key(k), value(v) {}
    QString key;
    QString value;
};

Param::Param() :
    d(0)
{

}

Param::Param(const QString &key, const QString &value) :
    d(new Data(key, value))
{

}

bool Param::isNull() const
{
    return d;
}

QString Param::key() const
{
    return isNull() ? QString() : d->key;
}

QString Param::value() const
{
    return isNull() ? QString() : d->value;
}

struct Base::Data : public QSharedData
{
    Data(const QUrl &b) : base(b) {}

    static void addQueryItem(QUrl *url, const Param &param)
    {
        if(!param.isNull())
        {
            url->addQueryItem(param.key(), param.value());
        }
    }

    QUrl base;
};

RecordList Base::find(FindStyle style, const QString &from,
                      const Param &first, const Param &second,
                      const Param &third, const Param &fourth)
{
    QUrl url = d->base;
    Data::addQueryItem(&url, first);
    Data::addQueryItem(&url, second);
    Data::addQueryItem(&url, third);
    Data::addQueryItem(&url, fourth);

    QNetworkAccessManager net;
    QNetworkReply *reply = net.get(QNetworkRequest(url));

    return RecordList();
}

Base::Base(const QUrl &base) :
    d(new Data(base))
{

}


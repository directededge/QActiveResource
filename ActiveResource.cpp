/*
 * Copyright (C) 2010, Directed Edge, Inc.
 */

#include "ActiveResource.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QXmlStreamReader>

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

RecordList Base::find(FindStyle style, const QString &from, const QList<Param> &params)
{
    QUrl url = d->base;

    foreach(Param param, params)
    {
        if(!param.isNull())
        {
            url.addQueryItem(param.key(), param.value());
        }
    }

    QNetworkAccessManager net;
    QNetworkReply *reply = net.get(QNetworkRequest(url));
    QXmlStreamReader xml(reply);

    return RecordList();
}

RecordList Base::find(FindStyle style, const QString &from,
                      const Param &first, const Param &second,
                      const Param &third, const Param &fourth)
{
    return find(style, from, QList<Param>() << first << second << third << fourth);
}

Base::Base(const QUrl &base) :
    d(new Data(base))
{

}


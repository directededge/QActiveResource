/*
 * Copyright (C) 2010, Directed Edge, Inc.
 */

#include "ActiveResource.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QXmlStreamReader>

#include <QDebug>

using namespace ActiveResource;

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
    return d == 0;
}

QString Param::key() const
{
    return isNull() ? QString() : d->key;
}

QString Param::value() const
{
    return isNull() ? QString() : d->value;
}

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

    qDebug() << url;

    while(reply->waitForReadyRead(30 * 1000 /* 30 seconds */) || reply->isRunning())
    {
        qDebug() << reply->readAll();
    }

    if(reply->error() != QNetworkReply::NoError)
    {
        qDebug() << reply->errorString();
    }

    // QXmlStreamReader xml(reply);

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



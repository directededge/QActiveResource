/*
 * Copyright (C) 2010, Directed Edge, Inc.
 */

#include "ActiveResource.h"
#include <QXmlStreamReader>
#include <QDebug>
#include <curl/curl.h>

using namespace ActiveResource;

static size_t writer(void *ptr, size_t size, size_t nmemb, void *stream)
{
    reinterpret_cast<QByteArray *>(stream)->append(QByteArray((const char *)(ptr), size * nmemb));
    return size * nmemb;
}

namespace HTTP
{
    QByteArray get(const QUrl &url)
    {
        QByteArray data;
        CURL *curl = curl_easy_init();

        static const int maxRetry = 1;
        int retry = 0;

        if(curl)
        {
            do
            {
                curl_easy_setopt(curl, CURLOPT_URL, url.toEncoded().data());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &data);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 25);
                curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
            }
            while(curl_easy_perform(curl) != 0 && ++retry <= maxRetry);
            curl_easy_cleanup(curl);
        }

        return data;
    }
}

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



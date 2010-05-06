/*
 * Copyright (C) 2010, Directed Edge, Inc. | Licensed under the MPL and LGPL
 */

#include "QActiveResource.h"
#include <QXmlStreamReader>
#include <QStringList>
#include <QDateTime>
#include <QDebug>
#include <curl/curl.h>

using namespace QActiveResource;

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

static QVariant::Type lookupType(const QString &name)
{
    static QHash<QString, QVariant::Type> types;

    if(types.isEmpty())
    {
        types["integer"] = QVariant::Int;
        types["decimal"] = QVariant::Double;
        types["datetime"] = QVariant::DateTime;
        types["boolean"] = QVariant::Bool;
    }

    return types.contains(name) ? types[name] : QVariant::String;
}

static void assign(Record *record, QString name, const QVariant &value)
{
    (*record)[name.replace('-', '_')] = value;
}

static QVariant reader(QXmlStreamReader &xml, bool advance = true)
{
    Record record;
    QString firstElement;

    while(!xml.atEnd())
    {
        if(advance)
        {
            xml.readNext();
        }
        if(xml.tokenType() == QXmlStreamReader::StartElement)
        {
            if(firstElement.isNull())
            {
                firstElement = xml.name().toString();
            }

            QString type = xml.attributes().value("type").toString();

            if(type == "array")
            {
                QString name = xml.name().toString();
                QList<QVariant> array;

                while(xml.readNext() == QXmlStreamReader::StartElement ||
                      (xml.tokenType() == QXmlStreamReader::Characters && xml.isWhitespace()))
                {
                    if(!xml.isWhitespace())
                    {
                        array.append(reader(xml, false));
                    }
                }

                assign(&record, name, array);
            }
            else if(xml.attributes().value("nil") == "true")
            {
                assign(&record, xml.name().toString(), QVariant());
            }
            else if(advance && xml.name() != firstElement)
            {
                QVariant value;
                QString text = xml.readElementText();

                switch(lookupType(type))
                {
                case QVariant::Int:
                    value = text.toInt();
                    break;
                case QVariant::Double:
                    value = text.toDouble();
                    break;
                case QVariant::DateTime:
                    value = QDateTime::fromString(text, Qt::ISODate);
                    break;
                case QVariant::Bool:
                    value = bool(text == "true");
                    break;
                default:
                    value = text;
                }

                assign(&record, xml.name().toString(), value);
            }
        }
        if(xml.tokenType() == QXmlStreamReader::EndElement &&
           !firstElement.isNull() && firstElement == xml.name())
        {
            return record;
        }

        advance = true;
    }

    return QVariant();
}

static RecordList fetch(QUrl url)
{
    if(!url.path().endsWith(".xml"))
    {
        url.setPath(url.path() + ".xml");
    }

    QByteArray data = HTTP::get(url);

    QXmlStreamReader xml(data);

    QVariant value = reader(xml);
    RecordList records;

    if(value.type() == QVariant::List)
    {
        foreach(QVariant v, value.toList())
        {
            records.append(v.toHash());
        }
    }
    else if(value.type() == QVariant::Hash && value.toHash().size() == 1)
    {
        foreach(QVariant v, value.toHash().begin()->toList())
        {
            records.append(v.toHash());
        }
    }
    else
    {
        records.append(value.toHash());
    }

    return records;
}

/*
 * Param::Data
 */

Param::Data::Data(const QString &k, const QString &v) :
    key(k),
    value(v)
{

}

/*
 * Param
 */

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

/*
 * Resource::Data
 */

Resource::Data::Data(const QUrl &b, const QString &r) :
    base(b),
    resource(r),
    url(base)
{
    setUrl();
}

void Resource::Data::setUrl()
{
    url = base;
    url.setPath(base.path() + "/" + resource);
}

/*
 * Resource
 */

Resource::Resource(const QUrl &base, const QString &resource) :
    d(new Data(base, resource))
{

}

void Resource::setBase(const QUrl &base)
{
    d->base = base;
    d->setUrl();
}

void Resource::setResource(const QString &resource)
{
    d->resource = resource;
    d->setUrl();
}

Record Resource::find(const QVariant &id) const
{
    return find(FindOne, id.toString());
}

RecordList Resource::find(FindMulti style, const QString &from, const QList<Param> &params) const
{
    Q_UNUSED(style);

    QUrl url = d->url;

    foreach(Param param, params)
    {
        if(!param.isNull())
        {
            url.addQueryItem(param.key(), param.value());
        }
    }

    if(!from.isEmpty())
    {
        url.setPath(url.path() + "/" + from);
    }

    return fetch(url);
}

Record Resource::find(FindSingle style, const QString &from, const QList<Param> &params) const
{
    QUrl url = d->url;

    foreach(Param param, params)
    {
        if(!param.isNull())
        {
            url.addQueryItem(param.key(), param.value());
        }
    }

    if(!from.isEmpty())
    {
        url.setPath(url.path() + "/" + from);
    }

    switch(style)
    {
    case FindOne:
    case FindFirst:
    {
        RecordList results = find(FindAll, from, params);
        return results.isEmpty() ? Record() : results.front();
    }
    case FindLast:
    {
        RecordList results = find(FindAll, from, params);
        return results.isEmpty() ? Record() : results.back();
    }
    }

    return Record();
}

Record Resource::find(FindSingle style, const QString &from,
                      const Param &first, const Param &second,
                      const Param &third, const Param &fourth) const
{
    return find(style, from, QList<Param>() << first << second << third << fourth);
}

RecordList Resource::find(FindMulti style, const QString &from,
                          const Param &first, const Param &second,
                          const Param &third, const Param &fourth) const
{
    return find(style, from, QList<Param>() << first << second << third << fourth);
}

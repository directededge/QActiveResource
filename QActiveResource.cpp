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
    QByteArray get(const QUrl &url, bool followRedirects = false)
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

                if(followRedirects)
                {
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
                    curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1);
                    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 2);
                }
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
    if(name == QACTIVERESOURCE_CLASS_KEY)
    {
        QString className = value.toString();
        QRegExp re("(^|-)([a-z])");

        while(className.indexOf(re) >= 0)
        {
            className.replace(re.cap(0), re.cap(2).toUpper());
        }

        (*record)[name] = className;
    }
    else
    {
        (*record)[name.replace('-', '_')] = value;
    }
}

static QDateTime toDateTime(const QString &s)
{
    QDateTime time = QDateTime::fromString(s.left(s.length() - 6), Qt::ISODate);

    time.setTimeSpec(Qt::UTC);

    int zoneHours = s.mid(s.length() - 6, 3).toInt();
    int zoneMinutes = s.right(2).toInt();

    return time.addSecs(-1 * (60 * zoneHours + zoneMinutes) * 60);
}

static QVariant reader(QXmlStreamReader &xml, bool advance = true)
{
    Record record;
    QString elementName;

    while(!xml.atEnd())
    {
        if(advance)
        {
            xml.readNext();
        }
        if(xml.tokenType() == QXmlStreamReader::StartElement)
        {
            if(elementName.isNull())
            {
                elementName = xml.name().toString();
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
            else if(advance && xml.name() != elementName)
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
                    value = toDateTime(text);
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
           !elementName.isNull() && elementName == xml.name())
        {
            assign(&record, QACTIVERESOURCE_CLASS_KEY, elementName);
            return record;
        }

        advance = true;
    }

    return QVariant();
}

static RecordList fetch(QUrl url, bool followRedirects = false)
{
    if(!url.path().endsWith(".xml"))
    {
        url.setPath(url.path() + ".xml");
    }

    QByteArray data = HTTP::get(url, followRedirects);

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
    else if(value.type() == QVariant::Hash && value.toHash().size() == 2)
    {
        foreach(QVariant v, value.toHash().begin()->toList())
        {
            records.append(v.toHash());
        }
    }
    else if(value.isValid())
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
    url(base),
    followRedirects(false)
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

RecordList Resource::find(FindMulti style, const QString &from, const ParamList &params) const
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

    return fetch(url, d->followRedirects);
}

Record Resource::find(FindSingle style, const QString &from, const ParamList &params) const
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
    return find(style, from, ParamList() << first << second << third << fourth);
}

RecordList Resource::find(FindMulti style, const QString &from,
                          const Param &first, const Param &second,
                          const Param &third, const Param &fourth) const
{
    return find(style, from, ParamList() << first << second << third << fourth);
}

void Resource::setFollowRedirects(bool followRedirects)
{
    d->followRedirects = followRedirects;
}

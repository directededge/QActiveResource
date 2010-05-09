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

static const QString QActiveResourceClassKey = "QActiveResource Class";

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
        int result = 0;

        if(curl)
        {
            QByteArray encodedUrl = url.toEncoded();
            QByteArray errorBuffer(CURL_ERROR_SIZE, 0);

            do
            {
                curl_easy_setopt(curl, CURLOPT_URL, encodedUrl.data());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &data);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 25);
                curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
                curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer.data());

                if(followRedirects)
                {
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
                    curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1);
                    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 2);
                }
            }
            while((result = curl_easy_perform(curl)) != 0 && ++retry <= maxRetry);

            if(result != 0)
            {
                qDebug() << "libcurl error" << result << errorBuffer << "for" << url;
            }

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

static QDateTime toDateTime(const QString &s)
{
    QDateTime time = QDateTime::fromString(s.left(s.length() - 6), Qt::ISODate);

    time.setTimeSpec(Qt::UTC);

    int zoneHours = s.mid(s.length() - 6, 3).toInt();
    int zoneMinutes = s.right(2).toInt();

    return time.addSecs(-1 * (60 * zoneHours + zoneMinutes) * 60);
}

static QString toClassName(QString name)
{
    if(name.isEmpty())
    {
        return name;
    }

    name[0] = name[0].toUpper();

    QRegExp re("-([a-z])");

    while(name.indexOf(re) >= 0)
    {
        name.replace(re.cap(0), re.cap(1).toUpper());
    }

    return name;
}

static QVariant extractFromRecord(const QVariant &record)
{
    QVariantHash hash = record.toHash();
    QStringList keys = hash.keys();
    keys.removeOne(QActiveResourceClassKey);
    return hash[keys.front()];
}

static bool hasNext(const QXmlStreamReader &xml)
{
    return (xml.tokenType() == QXmlStreamReader::StartElement ||
            (xml.tokenType() == QXmlStreamReader::Characters && xml.isWhitespace()));
}

static QVariant reader(QXmlStreamReader &xml, bool advance, bool isHash)
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
                QVariantList array;

                while(xml.readNext() && hasNext(xml))
                {
                    if(!xml.isWhitespace())
                    {
                        array.append(reader(xml, false, false));
                    }
                }

                assign(&record, name, array);
            }
            else if(xml.attributes().value("nil") == "true")
            {
                assign(&record, xml.name().toString(), QVariant());
            }
            else if((advance && xml.name() != elementName) || isHash)
            {
                QString name = xml.name().toString();

                while(xml.readNext() && xml.isWhitespace()) {}

                if(xml.tokenType() == QXmlStreamReader::StartElement)
                {
                    Record sub;
                    sub.setClassName(toClassName(name));

                    while(hasNext(xml))
                    {
                        if(!xml.isWhitespace())
                        {
                            QString subName = xml.name().toString();
                            assign(&sub, subName, extractFromRecord(reader(xml, false, true)));
                        }
                        xml.readNext();
                    }

                    assign(&record, name, sub);
                }
                else
                {
                    QVariant value;
                    QString text = xml.text().toString();

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
                        value = text.isEmpty() ? QVariant() : text;
                    }

                    assign(&record, name, value);
                }
            }
        }
        if(xml.tokenType() == QXmlStreamReader::EndElement &&
           !elementName.isNull() && elementName == xml.name())
        {
            record.setClassName(toClassName(elementName));
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

    QVariant value = reader(xml, true, false);
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
        foreach(QVariant v, extractFromRecord(value).toList())
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
 * Record
 */

Record::Record(const QVariantHash &hash) :
    d(new Data)
{
    d->hash = hash;
    d->className = d->hash.take(QActiveResourceClassKey).toString();
}

Record::Record(const QVariant &v) :
    d(new Data)
{
    d->hash = v.toHash();
    d->className = d->hash.take(QActiveResourceClassKey).toString();
}

QVariant &Record::operator[](const QString &key)
{
    return d->hash[key];
}

QVariant Record::operator[](const QString &key) const
{
    return d->hash[key];
}

bool Record::isEmpty() const
{
    return d->hash.isEmpty();
}

QString Record::className() const
{
    return d->className;
}

void Record::setClassName(const QString &name)
{
    d->className = name;
}

Record::operator QVariant() const
{
    QVariantHash hash = d->hash;
    hash[QActiveResourceClassKey] = d->className;
    return hash;
}

Record::ConstIterator Record::begin() const
{
    return d->hash.begin();
}

Record::ConstIterator Record::end() const
{
    return d->hash.end();
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
    url.setPath(base.path() + (base.path().endsWith("/") ? "" : "/") + resource);
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

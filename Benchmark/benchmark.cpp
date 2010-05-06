#include <QActiveResource.h>
#include <QDebug>

int main()
{
    qDebug() << getenv("AR_BASE");
    QActiveResource::Resource resource(QUrl(getenv("AR_BASE")), getenv("AR_RESOURCE"));

    const QString field = getenv("AR_FIELD");

    for(int i = 0; i < 100; i++)
    {
        qDebug() << i;
        foreach(QActiveResource::Record record, resource.find())
        {
            qDebug() << record[field].toString();
        }
    }

    return 0;
}

#include <QActiveResource.h>

int main(int argc, char *argv[])
{
    Q_ASSERT(getenv("AR_BASE") && getenv("AR_RESOURCE") && getenv("AR_FIELD"));

    int count = 100;

    if(argc > 1)
    {
        count = QString(argv[1]).toInt();
    }

    QActiveResource::Resource resource(QUrl(getenv("AR_BASE")), getenv("AR_RESOURCE"));

    const QString field = getenv("AR_FIELD");

    for(int i = 1; i <= count; i++)
    {
        printf("%i\n", i);

        foreach(QActiveResource::Record record, resource.find())
        {
            printf("%s\n", record[field].toString().toUtf8().data());
        }
    }

    return 0;
}

#include <QCoreApplication>
#include "lifecycletool.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    if( a.arguments().size() < 2 ) {
        qDebug() << "Drag and drop the project Life Cycle Tool Document folder on the application";
        exit(1);
    }
    LifeCycleTool lifeCycleTool;
    if( ! lifeCycleTool.readFile( a.arguments().at(1) ) )
    {
        qDebug() << "Failed...";
        exit(2);
    }

    return 0;
    return a.exec();
}

#include <QCoreApplication>
#include <QString>
#include <QDebug>
#include "lifecycletool.h"
#include "testrail.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	if( a.arguments().size() < 2 ) {
		qDebug() << "Drag and drop the exported TestRail xml OR project Life Cycle Tool Document folder on the application";
		exit(1);
	}
	if( a.arguments().at(1).endsWith( ".xml", Qt::CaseInsensitive ) )
	{
		TestRail testRail;
		if( ! testRail.readFile( a.arguments().at(1) ) )
		{
			qDebug() << "Failed...";
			exit(2);
		}
	}
	else
	{
		LifeCycleTool lifeCycleTool;
		if( ! lifeCycleTool.readFile( a.arguments().at(1) ) )
		{
			qDebug() << "Failed...";
			exit(2);
		}
	}

	system( "pause" );
	return 0;
	return a.exec();
}

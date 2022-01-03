#include <QCoreApplication>
#include <QFile>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QXmlStreamWriter>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    if( a.arguments().size() < 2 ) {
        qDebug() << "Drag and drop the project Life Cycle Tool Document folder on the application";
        exit(1);
    }
    const QString path = a.arguments().at(1);
    QFile order( path + "/requirements/order.json" );
    if( ! order.open(QFile::ReadOnly) )
    {
        qDebug() << "Could not open" << order.fileName();
        exit(2);
    }
    QJsonDocument const & requirementDocument = QJsonDocument::fromJson( order.readAll() );
    order.close();
    if( ! requirementDocument.isObject() )
    {
        qDebug() << "Bad JSON file (" << order.fileName() << ") as it does not contain a JSON object";
        exit(3);
    }
    QJsonObject const & requirements = requirementDocument.object();
    if( ! requirements.contains( "order" ) )
    {
        qDebug() << "Bad JSON file (" << order.fileName() << ") as it does not contain the key 'order'";
        exit(4);
    }
    QJsonValue const & requirementsOrder = requirements.value( "order" );
    if( ! requirementsOrder.isArray() )
    {
        qDebug() << "Bad JSON file (" << order.fileName() << ") as 'order' is not an array";
        exit(5);
    }
    QFile testRailFile( path + "/testrail.xml" );
    if( ! testRailFile.open(QFile::WriteOnly) )
    {
        qDebug() << "Could not create" << testRailFile.fileName();
        exit(10);
    }
    QXmlStreamWriter testRail( &testRailFile );
    testRail.setAutoFormatting( true );
    testRail.writeStartDocument();
//    testRail.writeStartElement( "suite" );
//    testRail.writeTextElement( "name", "Life Cycle Tool" );
//    testRail.writeTextElement( "description", "Imported by LCTRL v1.0 from " + path );
    testRail.writeStartElement( "sections" );
    testRail.writeStartElement( "section" );
    testRail.writeTextElement( "name", "Import" );
    testRail.writeTextElement( "description", "Imported by LCTRL v1.0 on " + QDateTime::currentDateTimeUtc().toString() + " from " + path );
    int level = 0;
    foreach( QJsonValue const & item, requirementsOrder.toArray() )
    {
        QStringList parts = item.toString().split( '.' );
        QFile file( path + "/requirements/" + parts.last() + ".json" );
        if( ! file.open(QFile::ReadOnly) )
        {
            qDebug() << "Could not open" << file.fileName();
            exit(6);
        }
        QJsonDocument const & document = QJsonDocument::fromJson( file.readAll() );
        file.close();
        if( ! document.isObject() )
        {
            qDebug() << "Bad JSON file (" << file.fileName() << ") as it does not contain a JSON object";
            exit(7);
        }
        QJsonObject const & object = document.object();
        if( ! object.contains( "name" ) || ! object.contains( "description" ) || ! object.contains( "type" ) )
        {
            qDebug() << "Bad JSON file (" << file.fileName() << ") as it does not contain the key 'name' or 'description' or 'type'";
            exit(8);
        }
        QJsonValue const & name = object.value( "name" );
        QJsonValue const & description = object.value( "description" );
        QJsonValue const & type = object.value( "type" );
        if( ! name.isString() || ! type.isString() )
        {
            qDebug() << "Bad JSON file (" << file.fileName() << ") as 'name' or 'type' is not a string";
            exit(9);
        }
        QString const id = (type=="user_need"?"USN-":type=="requirement"?"REQ-":type=="specification"?"SPC-":"???-") + parts.last();
        qDebug() << ( parts.size() == 1 ? "" : parts.size() == 2 ? "\t" : "\t\t" ) << id;//name.toString();
        if( parts.size() <= level )
        {
            testRail.writeEndElement(); //section
        }
        while( parts.size() < level )
        {
            testRail.writeEndElement(); //sections
            testRail.writeEndElement(); //section
            level--;
        }
        while( parts.size() > level )
        {
            testRail.writeStartElement( "sections" );
            level++;
        }
        testRail.writeStartElement( "section" );
        testRail.writeTextElement( "name", name.toString() );
        testRail.writeTextElement( "description", "***" + id + "***: " + description.toString() );
        if( object.contains( "testIds" ) && object.value( "testIds" ).isArray() && ! object.value( "testIds" ).toArray().isEmpty() )
        {
            testRail.writeStartElement( "cases" );
            foreach( QJsonValue const & testId, object.value( "testIds" ).toArray() )
            {
                testRail.writeStartElement( "case" );
                QFile testFile( path + "/testCases/" + testId.toString() + ".json" );
                if( ! testFile.open(QFile::ReadOnly) )
                {
                    qDebug() << "Could not open" << testFile.fileName();
                    exit(11);
                }
                QJsonDocument const & testDocument = QJsonDocument::fromJson( testFile.readAll() );
                testFile.close();
                if( ! testDocument.isObject() )
                {
                    qDebug() << "Bad JSON file (" << testFile.fileName() << ") as it does not contain a JSON object";
                    exit(12);
                }
                QJsonObject const & test = testDocument.object();
                if( ! test.contains( "name" ) || ! test.contains( "testSteps" ) || ! test.contains( "expectedResult" ) || ! test.contains( "type" ) )
                {
                    qDebug() << "Bad JSON file (" << testFile.fileName() << ") as it does not contain the key 'name' or 'description' or 'type'";
                    exit(13);
                }
                testRail.writeTextElement( "title", test.value( "name" ).toString() );
                testRail.writeTextElement( "template", "Test Case (Steps)" );
                testRail.writeTextElement( "type", "Acceptance" );
                testRail.writeTextElement( "priority", "Medium" );
//                testRail.writeTextElement( "estimate", "" );
                testRail.writeTextElement( "references", item.toString() + "." + testId.toString() );
                testRail.writeStartElement( "custom" );
                testRail.writeStartElement( "automation_type" );
                if( test.value( "type" ) == "manual" )
                {
                    testRail.writeTextElement( "id", "0" );
                    testRail.writeTextElement( "value", " None" );
                }
                else
                {
                    qDebug() << "---------------> Automated? : " << test.value( "type" ).toString();
                    testRail.writeTextElement( "id", "1" );
                    testRail.writeTextElement( "value", " LifeCycleTool" );
                }
                testRail.writeEndElement(); //automation_type
//                testRail.writeTextElement( "preconds", "" );
                testRail.writeStartElement( "steps_separated" );
                if( test.contains( "testSteps" ) && test.value( "testSteps" ).isArray() && ! test.value( "testSteps" ).toArray().isEmpty() )
                {
                    int index = 1;
                    foreach( QJsonValue const & step, test.value( "testSteps" ).toArray() )
                    {
                        testRail.writeStartElement( "step" );
                        testRail.writeTextElement( "index", QString::number(index) );
                        testRail.writeTextElement( "content", step.toString() );
                        if( test.value( "testSteps" ).toArray().size() == index )
                        {
                            testRail.writeTextElement( "expected", test.value( "expectedResult" ).toString() );
                        }
                        testRail.writeEndElement(); //step
                        index++;
                    }
                }
                else
                {
                    testRail.writeStartElement( "step" );
                    testRail.writeTextElement( "index", "1" );
                    testRail.writeTextElement( "content", "" );
                    testRail.writeTextElement( "expected", test.value( "expectedResult" ).toString() );
                    testRail.writeEndElement(); //step
                }
                testRail.writeEndElement(); //steps_separated
                testRail.writeEndElement(); //custom
                testRail.writeEndElement(); //case
            }
            testRail.writeEndElement(); //cases
        }
    }
    testRail.writeEndElement(); //section
    while( 0 < level )
    {
        testRail.writeEndElement(); //sections
        testRail.writeEndElement(); //section
        level--;
    }
    testRail.writeEndElement(); //sections Life Cycle Tool
//    testRail.writeEndElement(); //suite
    testRail.writeEndDocument();
    testRailFile.close();


    //qDebug() << requirements.toJson(QJsonDocument::Compact);
    return 0;
    return a.exec();
}

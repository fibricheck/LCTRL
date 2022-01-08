#include "lifecycletool.h"
#include <QFile>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QXmlStreamWriter>

LifeCycleTool::LifeCycleTool()
{
}

bool LifeCycleTool::readFile( const QString & path )
{
    QFile order( path + "/requirements/order.json" );
    if( ! order.open(QFile::ReadOnly) )
    {
        qDebug() << "Could not open" << order.fileName();
        return false;
    }
    QJsonDocument const & requirementDocument = QJsonDocument::fromJson( order.readAll() );
    order.close();
    if( ! requirementDocument.isObject() )
    {
        qDebug() << "Bad JSON file (" << order.fileName() << ") as it does not contain a JSON object";
        return false;
    }
    QJsonObject const & requirements = requirementDocument.object();
    if( ! requirements.contains( "order" ) )
    {
        qDebug() << "Bad JSON file (" << order.fileName() << ") as it does not contain the key 'order'";
        return false;
    }
    QJsonValue const & requirementsOrder = requirements.value( "order" );
    if( ! requirementsOrder.isArray() )
    {
        qDebug() << "Bad JSON file (" << order.fileName() << ") as 'order' is not an array";
        return false;
    }
    QFile testRailFile( path + "/testrail.xml" );
    if( ! testRailFile.open(QFile::WriteOnly) )
    {
        qDebug() << "Could not create" << testRailFile.fileName();
        return false;
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
            return false;
        }
        QJsonDocument const & document = QJsonDocument::fromJson( file.readAll() );
        file.close();
        if( ! document.isObject() )
        {
            qDebug() << "Bad JSON file (" << file.fileName() << ") as it does not contain a JSON object";
            return false;
        }
        QJsonObject const & object = document.object();
        if( ! object.contains( "name" ) || ! object.contains( "description" ) || ! object.contains( "type" ) )
        {
            qDebug() << "Bad JSON file (" << file.fileName() << ") as it does not contain the key 'name' or 'description' or 'type'";
            return false;
        }
        QJsonValue const & name = object.value( "name" );
        QJsonValue const & description = object.value( "description" );
        QJsonValue const & type = object.value( "type" );
        if( ! name.isString() || ! type.isString() )
        {
            qDebug() << "Bad JSON file (" << file.fileName() << ") as 'name' or 'type' is not a string";
            return false;
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
                    return false;
                }
                QJsonDocument const & testDocument = QJsonDocument::fromJson( testFile.readAll() );
                testFile.close();
                if( ! testDocument.isObject() )
                {
                    qDebug() << "Bad JSON file (" << testFile.fileName() << ") as it does not contain a JSON object";
                    return false;
                }
                QJsonObject const & test = testDocument.object();
                if( ! test.contains( "name" ) || ! test.contains( "testSteps" ) || ! test.contains( "expectedResult" ) || ! test.contains( "type" ) )
                {
                    qDebug() << "Bad JSON file (" << testFile.fileName() << ") as it does not contain the key 'name' or 'description' or 'type'";
                    return false;
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

    return true;
}

#include "testrail.h"
#include <QtDebug>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QRandomGenerator>

TestRail::TestRail()
{
}

bool TestRail::readFile( const QString & fileName )
{
    QFile file( fileName );
    if( ! file.open( QFile::ReadOnly ) )
    {
        qDebug() << "Could not open" << file.fileName();
        return false;
    }
    path = QFileInfo( fileName ).path();
    if( ! needsDirectory( path + "/Documentation" ) ||
        ! needsDirectory( path + "/Documentation/requirements" ) ||
        ! needsDirectory( path + "/Documentation/testCases" ) )
    {
        return false;
    }
    xml.setDevice( &file );
    if( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String( "suite" ) )
        {
            xml.readNextStartElement();
        }
        if( xml.name() == QLatin1String( "id" ) )
        {
            xml.skipCurrentElement();
            xml.readNextStartElement();
        }
        if( xml.name() == QLatin1String( "name" ) )
        {
            xml.skipCurrentElement();
            xml.readNextStartElement();
        }
        if( xml.name() == QLatin1String( "description" ) )
        {
            xml.skipCurrentElement();
            xml.readNextStartElement();
        }
        if( xml.name() == QLatin1String( "sections" ) )
        {
            readSections();
        }
        else
        {
            qDebug() << "Unknown tag in suite ( @" << xml.lineNumber() << ") --> " << xml.name();
            xml.raiseError( "The file does not contain a 'sections' tag." );
        }
    }
    if( xml.hasError() )
    {
        qDebug() << "Bad XML file (" << file.fileName() << ")" << xml.errorString();
    }
    file.close();
    return !xml.error();
}

bool TestRail::needsDirectory( const QString & directory )
{
    QDir dir( directory );
    if( ! dir.exists() )
    {
        if( ! dir.mkpath( "." ) )
        {
            qDebug() << "Could not create" << dir.path();
            return false;
        }
    }
    return true;
}

void TestRail::readSections()
{
    Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "sections" ) );

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String( "section" ) )
        {
            readSection();
        }
        else
        {
            qDebug() << "Unknown tag in sections ( @" << xml.lineNumber() << ") --> " << xml.name();
            xml.skipCurrentElement();
        }
    }
}

void TestRail::readSection()
{
    Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "section" ) );

    QString id;
    QString name;
    QString description;
    QString type;
    QStringList testIds;
//    QString origin;

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String( "name" ) )
        {
            name = xml.readElementText().trimmed().replace( '"', "\\\"" ).replace( '\n', "\\n" );
        }
        else if( xml.name() == QLatin1String( "description" ) )
        {
            const QString & idAndDescription = xml.readElementText();
            int index = idAndDescription.indexOf( "***:" );
            if( index > 0 )
            {
                if( idAndDescription.startsWith( "***USN-" ) )
                {
                    id = idAndDescription.mid(7, index-7);
                    if( id.contains( '?' ) )
                    {
                        id = generateId( 5 );
                    }
                    description = idAndDescription.mid(index+4).trimmed().replace( '"', "\\\"" ).replace( '\n', "\\n" );
                    type = "user_need";
//                    origin = "user_need";
                }
                else if( idAndDescription.startsWith( "***REQ-" ) )
                {
                    id = idAndDescription.mid(7, index-7);
                    if( id.contains( '?' ) )
                    {
                        id = generateId( 5 );
                    }
                    description = idAndDescription.mid(index+4).trimmed().trimmed().replace( '"', "\\\"" ).replace( '\n', "\\n" );
                    type = "requirement";
//                    origin = "user_need";
                }
                else if( idAndDescription.startsWith( "***SPC-" ) )
                {
                    id = idAndDescription.mid(7, index-7);
                    if( id.contains( '?' ) )
                    {
                        id = generateId( 5 );
                    }
                    description = idAndDescription.mid(index+4).trimmed().trimmed().replace( '"', "\\\"" ).replace( '\n', "\\n" );
                    type = "specification";
//                    origin = "user_need";
                }
                else
                {
                    qDebug() << "Badly formed description, the id must be of the form of ***XXX-?????***: ( @" << xml.lineNumber() << ") --> " << idAndDescription << index;
                }
            }
            else if( idAndDescription.contains( "USN-" ) || idAndDescription.contains( "REQ-" ) || idAndDescription.contains( "SPC-" ) )
            {
                qDebug() << "Badly formed description, the id must be of the form of ***XXX-?????***: ( @" << xml.lineNumber() << ") --> " << idAndDescription;
            }
        }
        else if( xml.name() == QLatin1String( "sections" ) )
        {
            readSections();
        }
        else if( xml.name() == QLatin1String( "cases" ) )
        {
            testIds = readCases();
        }
        else
        {
            qDebug() << "Unknown tag in section ( @" << xml.lineNumber() << ") --> " << xml.name();
            xml.skipCurrentElement();
        }
    }

    if( ! id.isEmpty() )
    {
        QFile json( path + "/Documentation/requirements/" + id + ".json" );
        if( ! json.open( QFile::WriteOnly ) )
        {
            qDebug() << "Could not create" << json.fileName();
        }
        else
        {
            qDebug() << "Creating" << json.fileName();
            QString testIdsJson = "";
            if( testIds.size() > 0 )
            {
                testIdsJson = QString( ",\n  \"testIds\": [\n    \"%1\"\n  ]" ).arg( testIds.join("\",\n    \"") );
            }
            json.write( QString( "{\n"
                        "  \"id\": \"%1\",\n"
                        "  \"name\": \"%2\",\n"
                        "  \"description\": \"%3\",\n"
                        "  \"type\": \"%4\",\n"
                        "  \"origin\": \"user_need\"%5\n"
                        "}" ).arg( id ).arg( name ).arg( description ).arg( type ).arg( testIdsJson ).toUtf8()
                        );
            json.close();
        }
    }
}

QStringList TestRail::readCases()
{
    Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "cases" ) );

    QStringList testIds;

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String( "case" ) )
        {
            testIds.append( readCase() );
        }
        else
        {
            qDebug() << "Unknown tag in cases ( @" << xml.lineNumber() << ") --> " << xml.name();
            xml.skipCurrentElement();
        }
    }
    return testIds;
}

QString TestRail::readCase()
{
    Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "case" ) );

    QString id;

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String( "id" ) ||
            xml.name() == QLatin1String( "title" ) ||
            xml.name() == QLatin1String( "template" ) ||
            xml.name() == QLatin1String( "type" ) ||
            xml.name() == QLatin1String( "priority" ) ||
            xml.name() == QLatin1String( "estimate" ) )
        {
            xml.readElementText();
        }
        else if( xml.name() == QLatin1String( "references" ) )
        {
            id = xml.readElementText().split('.').last();
        }
        else if( xml.name() == QLatin1String( "custom" ) )
        {
            readCustom();
        }
        else
        {
            qDebug() << "Unknown tag in case ( @" << xml.lineNumber() << ") --> " << xml.name();
            xml.skipCurrentElement();
        }
    }
    return id;
}

void TestRail::readCustom()
{
    Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "custom" ) );

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String( "automation_type" ) )
        {
            readAutomationType();
        }
        else if( xml.name() == QLatin1String( "steps_separated" ) )
        {
            readStepsSeperated();
        }
        else
        {
            qDebug() << "Unknown tag in custom ( @" << xml.lineNumber() << ") --> " << xml.name();
            xml.skipCurrentElement();
        }
    }
}

void TestRail::readAutomationType()
{
    Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "automation_type" ) );

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String( "id" ) )
        {
            switch( xml.readElementText().toInt() )
            {
            case 0:
                //Manual
                break;
            case 1:
                //Automated
                break;
            }
        }
        else if( xml.name() == QLatin1String( "value" ) )
        {
            xml.readElementText();
        }
        else
        {
            qDebug() << "Unknown tag in section ( @" << xml.lineNumber() << ") --> " << xml.name();
            xml.skipCurrentElement();
        }
    }
}

void TestRail::readStepsSeperated()
{
    Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "steps_separated" ) );

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String( "step" ) )
        {
            readStep();
        }
        else
        {
            qDebug() << "Unknown tag in steps_separated ( @" << xml.lineNumber() << ") --> " << xml.name();
            xml.skipCurrentElement();
        }
    }
}

void TestRail::readStep()
{
    Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "step" ) );

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String( "index" ) )
        {
            xml.readElementText();
        }
        else if( xml.name() == QLatin1String( "content" ) )
        {
            xml.readElementText();
        }
        else if( xml.name() == QLatin1String( "expected" ) )
        {
            xml.readElementText();
        }
        else
        {
            qDebug() << "Unknown tag in step ( @" << xml.lineNumber() << ") --> " << xml.name();
            xml.skipCurrentElement();
        }
    }
}

QString TestRail::generateId( qsizetype size )
{
    static const QString symbols( "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" );
    QString id( size, '?' );
    for( int i = 0 ; i < size ; ++i )
    {
        id[i] = symbols[ QRandomGenerator::global()->bounded( symbols.size() ) ];
    }
    return id;
}

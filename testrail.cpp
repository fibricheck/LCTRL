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
	todo.setFileName( path + "/Documentation/TODO.log" );
	if( ! todo.open( QFile::WriteOnly ) )
	{
		qDebug() << "Could not create" << todo.fileName();
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
			QStringList order = readSections( "", QStringList(), "" );
			QFile orderJson( path + "/Documentation/requirements/order.json" );
			if( ! orderJson.open( QFile::WriteOnly ) )
			{
				qDebug() << "Could not create" << orderJson.fileName();
			}
			else
			{
				orderJson.write( QString( "{\n  \"order\": [\n    \"%1\"\n  ]\n}" ).arg( order.join("\",\n    \"") ).toUtf8() );
			}
			orderJson.close();
			qDebug() << "Created" << orderJson.fileName();
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
	todo.close();
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

QStringList TestRail::readSections( const QString & parentId, QStringList parentNames, const QString & topNonParentName )
{
	Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "sections" ) );

	QStringList order;

	while( xml.readNextStartElement() )
	{
		if( xml.name() == QLatin1String( "section" ) )
		{
			order.append( readSection( parentId, parentNames, topNonParentName ) );
		}
		else
		{
			qDebug() << "Unknown tag in sections ( @" << xml.lineNumber() << ") --> " << xml.name();
			xml.skipCurrentElement();
		}
	}

	return order;
}

QStringList TestRail::readSection( const QString & parentId, QStringList parentNames, const QString & topNonParentName )
{
	Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "section" ) );

	QStringList order;

	QString id;
	QString name;
	QString description;
	QString type;
//    QString origin;
	QStringList testIds;

	while( xml.readNextStartElement() )
	{
		if( xml.name() == QLatin1String( "name" ) )
		{
			name = xml.readElementText().trimmed().replace( '\\', "\\\\" ).replace( '"', "\\\"" ).replace( '\n', "\\n" ).replace( '\t', "\\t" );
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
					description = idAndDescription.mid(index+4).trimmed().replace( '\\', "\\\\" ).replace( '"', "\\\"" ).replace( '\n', "\\n" ).replace( '\t', "\\t" );
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
					description = idAndDescription.mid(index+4).trimmed().trimmed().replace( '\\', "\\\\" ).replace( '"', "\\\"" ).replace( '\n', "\\n" ).replace( '\t', "\\t" );
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
					description = idAndDescription.mid(index+4).trimmed().trimmed().replace( '\\', "\\\\" ).replace( '"', "\\\"" ).replace( '\n', "\\n" ).replace( '\t', "\\t" );
					type = "specification";
//                    origin = "user_need";
				}
				else
				{
					qDebug() << "Badly formed description, the id must be of the form of ***XXX-?????***: ( @" << xml.lineNumber() << ") --> " << idAndDescription << index;
				}

				if( ! id.isEmpty() )
				{
					order.append( parentId + id );
					parentNames.append( name );
				}
			}
			else if( idAndDescription.contains( "USN-" ) || idAndDescription.contains( "REQ-" ) || idAndDescription.contains( "SPC-" ) )
			{
				qDebug() << "Badly formed description, the id must be of the form of ***XXX-?????***: ( @" << xml.lineNumber() << ") --> " << idAndDescription;
			}
		}
		else if( xml.name() == QLatin1String( "sections" ) )
		{
			order.append( readSections( id.isEmpty()?parentId:(parentId + id+'.'), parentNames, id.isEmpty()?name:"" ) );
		}
		else if( xml.name() == QLatin1String( "cases" ) )
		{
			testIds = readCases( parentNames );
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
		if( json.exists() )
		{
			qDebug() << "Duplicate id for " << json.fileName();
			todo.write( QString( "Duplicate requirement id : %1 \n" ).arg( id ).toUtf8() );
		}
		if( ! json.open( QFile::WriteOnly ) )
		{
			qDebug() << "Could not create" << json.fileName();
		}
		else
		{
			QString testIdsJson = "";
			if( testIds.size() > 0 )
			{
				testIdsJson = QString( ",\n  \"testIds\": [\n    \"%1\"\n  ]" ).arg( testIds.join("\",\n    \"") );
			}
			else if( type == "specification" )
			{
				todo.write( QString( "Specification without test cases : %1 \n" ).arg( id ).toUtf8() );
//				testIdsJson = ",\n  \"testIds\": []";
			}
			json.write( QString( "{\n"
						"  \"id\": \"%1\",\n"
						"  \"name\": \"%2\",\n"
						"  \"description\": \"%3\",\n"
						"  \"type\": \"%4\",\n"
						"  \"origin\": \"user_need\"%5\n"
						"}" ).arg( id ).arg( name.prepend( topNonParentName.isEmpty()?"":("["+topNonParentName+"] ") ) ).arg( description ).arg( type ).arg( testIdsJson ).toUtf8()
						);
			json.close();
			qDebug() << "Created" << json.fileName();
		}
	}

	return order;
}

QStringList TestRail::readCases( QStringList parentNames )
{
	Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "cases" ) );

	QStringList testIds;

	while( xml.readNextStartElement() )
	{
		if( xml.name() == QLatin1String( "case" ) )
		{
			QString id = readCase( parentNames );
			if( ! id.isEmpty() )
			{
				testIds.append( id );
			}
			else
			{
				qDebug() << "ID not set in cases ( @" << xml.lineNumber() << ") --> " << xml.name();
			}
		}
		else
		{
			qDebug() << "Unknown tag in cases ( @" << xml.lineNumber() << ") --> " << xml.name();
			xml.skipCurrentElement();
		}
	}
	return testIds;
}

QString TestRail::readCase( QStringList parentNames )
{
	Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "case" ) );

	QString id;
	QString name;
	QString type = "manual";
	QString priority = "Medium";
	QVector<QPair<QString,QString>> testSteps;
	QString testRailID;
	QString preconditions;

	while( xml.readNextStartElement() )
	{
		if( xml.name() == QLatin1String( "template" ) ||
			xml.name() == QLatin1String( "estimate" ) )
		{
			xml.readElementText();
		}
		else if( xml.name() == QLatin1String( "id" ) )
		{
			testRailID = xml.readElementText();
		}
		else if( xml.name() == QLatin1String( "title" ) )
		{
			name = xml.readElementText().trimmed().replace( '\\', "\\\\" ).replace( '"', "\\\"" ).replace( '\n', "\\n" ).replace( '\t', "\\t" );
		}
		else if( xml.name() == QLatin1String( "references" ) )
		{
			id = xml.readElementText().split('.').last();
		}
		else if( xml.name() == QLatin1String( "type" ) )
		{
			type = xml.readElementText().trimmed();
			if( type == "Other" )
			{
				type = "manual";
			}
		}
		else if( xml.name() == QLatin1String( "priority" ) )
		{
			priority = xml.readElementText();
		}
		else if( xml.name() == QLatin1String( "custom" ) )
		{
			testSteps = readCustom( preconditions );
		}
		else
		{
			qDebug() << "Unknown tag in case ( @" << xml.lineNumber() << ") --> " << xml.name();
			xml.skipCurrentElement();
		}
	}

	if( id.isEmpty() )
	{
		id = generateId( 5 );
	}

	if( ! name.isEmpty() )
	{
		QFile json( path + "/Documentation/testCases/" + id + ".json" );
		if( json.exists() )
		{
			qDebug() << "Duplicate id for " << json.fileName();
			todo.write( QString( "Duplicate testcase id : %1 \n" ).arg( id ).toUtf8() );
		}
		if( ! json.open( QFile::WriteOnly ) )
		{
			qDebug() << "Could not create" << json.fileName();
		}
		else
		{
			QString testStepsJson = "";
			QString expectedResult = "";
			if( testSteps.size() > 0 )
			{
				expectedResult = testSteps.last().second;
				testStepsJson = QString( ",\n  \"testSteps\": [\n    \"%1" ).arg( testSteps.first().first );
				if( ! preconditions.isEmpty() )
				{
					testStepsJson += "\\n\\n**preconditions:**\\n```\\n";
					testStepsJson += preconditions;
					testStepsJson += "\\n```";
				}
				for( int i = 1 ; i < testSteps.size() ; ++i )
				{
					if( ! testSteps.at(i-1).second.isEmpty() )
					{
						testStepsJson += "\\n\\n**assertion:**\\n```\\n";
						testStepsJson += testSteps.at(i-1).second;
						testStepsJson += "\\n```";
					}
					testStepsJson += "\",\n    \"";
					testStepsJson += testSteps.at(i).first;
				}
				testStepsJson += "\"\n  ]";
			}
			QString parentNamesJson = "";
			if( parentNames.size() > 0 )
			{
				parentNamesJson = QString( ",\n  \"parentNames\": [\n    \"%1\"\n  ]" ).arg( parentNames.join("\",\n    \"") );
			}
			if( name.compare( "Check", Qt::CaseInsensitive ) == 0 )
			{
				name.append( ' ' );
				name.append( parentNames.last() );
			}
			json.write( QString( "{\n"
						"  \"id\": \"%1\",\n"
						"  \"name\": \"%2\"%3,\n"
						"  \"expectedResult\": \"%4\",\n"
						"  \"type\": \"manual\",\n"
						"  \"tier\": \"%5\",\n"
						"  \"keyValues\": [\n"
						"    {\n"
						"      \"key\": \"testrail-id\",\n"
						"      \"value\": \"%6\"\n"
						"    },\n"
						"    {\n"
						"      \"key\": \"priority\",\n"
						"      \"value\": \"%7\"\n"
						"    }\n"
						"  ]%8\n"
						"}" ).arg( id ).arg( name ).arg( testStepsJson ).arg( expectedResult ).arg( type ).arg( testRailID ).arg( priority ).arg( parentNamesJson ).toUtf8()
						);
			json.close();
			qDebug() << "Created" << json.fileName();
		}
	}

	return id;
}

QVector<QPair<QString,QString>> TestRail::readCustom( QString & preconditions )
{
	Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "custom" ) );

	QVector<QPair<QString,QString>> testSteps;

	while( xml.readNextStartElement() )
	{
		if( xml.name() == QLatin1String( "automation_type" ) )
		{
			readAutomationType();
		}
		else if( xml.name() == QLatin1String( "preconds" ) )
		{
			preconditions = xml.readElementText().trimmed().replace( '\\', "\\\\" ).replace( '"', "\\\"" ).replace( '\n', "\\n" ).replace( '\t', "\\t" );
		}
		else if( xml.name() == QLatin1String( "steps_separated" ) )
		{
			testSteps = readStepsSeperated();
		}
		else
		{
			qDebug() << "Unknown tag in custom ( @" << xml.lineNumber() << ") --> " << xml.name();
			xml.skipCurrentElement();
		}
	}

	return testSteps;
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

QVector<QPair<QString,QString>> TestRail::readStepsSeperated()
{
	Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "steps_separated" ) );

	QVector<QPair<QString,QString>> testSteps;

	while( xml.readNextStartElement() )
	{
		if( xml.name() == QLatin1String( "step" ) )
		{
			testSteps.append( readStep() );
		}
		else
		{
			qDebug() << "Unknown tag in steps_separated ( @" << xml.lineNumber() << ") --> " << xml.name();
			xml.skipCurrentElement();
		}
	}

	return testSteps;
}

QPair<QString,QString> TestRail::readStep()
{
	Q_ASSERT( xml.isStartElement() && xml.name() == QLatin1String( "step" ) );

	QPair<QString,QString> testStep;

	while( xml.readNextStartElement() )
	{
		if( xml.name() == QLatin1String( "index" ) )
		{
			xml.readElementText();
		}
		else if( xml.name() == QLatin1String( "content" ) )
		{
			testStep.first = xml.readElementText().trimmed().replace( '\\', "\\\\" ).replace( '"', "\\\"" ).replace( '\n', "\\n" ).replace( '\t', "\\t" );
		}
		else if( xml.name() == QLatin1String( "expected" ) )
		{
			testStep.second = xml.readElementText().trimmed().replace( '\\', "\\\\" ).replace( '"', "\\\"" ).replace( '\n', "\\n" ).replace( '\t', "\\t" );
		}
		else
		{
			qDebug() << "Unknown tag in step ( @" << xml.lineNumber() << ") --> " << xml.name();
			xml.skipCurrentElement();
		}
	}

	return testStep;
}

QString TestRail::generateId( qsizetype size )
{
	static const QString symbols( "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" );
	QString id( size, '?' );
	for( int i = 0 ; i < size ; ++i )
	{
		id[i] = symbols[ QRandomGenerator::global()->bounded( symbols.size() ) ];
	}
	if( QFile::exists( path + "/Documentation/testCases/" + id + ".json" ) || QFile::exists( path + "/Documentation/requirements/" + id + ".json" ) )
	{
		qDebug() << "Regenerate ID" << id << "exists !";
		return generateId( size );
	}
	return id;
}

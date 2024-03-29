#ifndef TESTRAIL_H
#define TESTRAIL_H

#include <QFile>
#include <QXmlStreamReader>

class TestRail
{
public:
	TestRail();
	bool readFile( QString const & fileName );

private:
    bool needsDirectory( QString const & directory );
	QStringList readSections( const QString & parentId, QStringList parentNames, const QString & topNonParentName );
	QStringList readSection( const QString & parentId, QStringList parentNames, const QString & topNonParentName );
	QStringList readCases( QStringList parentNames, const QString & order );
	QString readCase( QStringList parentNames, const QString & order );
    QVector<QPair<QString, QString>> readCustom( QString & preconditions );
    void readAutomationType();
    QVector<QPair<QString, QString>> readStepsSeperated();
    QPair<QString, QString> readStep();
    QString generateId( qsizetype size );
	QFile todo;
    QXmlStreamReader xml;
    QString path;
};

#endif // TESTRAIL_H

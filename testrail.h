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
    QStringList readSections( const QString & parentId );
    QStringList readSection( const QString & parentId );
    QStringList readCases();
    QString readCase();
    void readCustom();
    void readAutomationType();
    void readStepsSeperated();
    void readStep();
    QString generateId( qsizetype size );
    QXmlStreamReader xml;
    QString path;
};

#endif // TESTRAIL_H

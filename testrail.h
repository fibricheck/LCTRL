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
    QStringList readSections( const QString & parentId, QStringList parentNames );
    QStringList readSection( const QString & parentId, QStringList parentNames );
    QStringList readCases( QStringList parentNames );
    QString readCase( QStringList parentNames );
    QVector<QPair<QString, QString>> readCustom();
    void readAutomationType();
    QVector<QPair<QString, QString>> readStepsSeperated();
    QPair<QString, QString> readStep();
    QString generateId( qsizetype size );
    QXmlStreamReader xml;
    QString path;
};

#endif // TESTRAIL_H

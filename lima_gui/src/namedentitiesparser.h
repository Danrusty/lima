#ifndef NAMEDENTITIESPARSER_H
#define NAMEDENTITIESPARSER_H

#include <QObject>
#include <QString>

struct EntityItem {

  EntityItem(std::string name, std::string color, const std::vector<std::string>& occ) :
  name(name), color(color), occurences(occ)
  {}

  std::string name;
  std::string color;
  std::vector<std::string> occurences;
};

class NamedEntitiesParser : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QString highlightedText READ getHighlightedText)
  Q_PROPERTY(QStringList entityTypes READ getEntityTypes)
public:
  NamedEntitiesParser(QObject* p = 0);

  Q_INVOKABLE void parse(const QString& rawText, const QString& conllText);

  Q_INVOKABLE QStringList getEntityTypes();
  Q_INVOKABLE QString getHighlightedText();

private:
  std::vector<EntityItem> entities;
  QString rawText;
  QString conllText;
};

#endif // NAMEDENTITIESPARSER_H

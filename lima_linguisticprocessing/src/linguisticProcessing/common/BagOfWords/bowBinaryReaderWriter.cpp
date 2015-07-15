/*
    Copyright 2002-2013 CEA LIST

    This file is part of LIMA.

    LIMA is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LIMA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with LIMA.  If not, see <http://www.gnu.org/licenses/>
*/
/************************************************************************
 *
 * @file       bowBinaryReaderWriter.cpp
 * @author     Romaric Besancon (romaric.besancon@cea.fr)
 * @date       Tue Mar 20 2007
 * copyright   Copyright (C) 2007 by CEA LIST
 *
 ***********************************************************************/

#include "bowBinaryReaderWriter.h"
#include "AbstractBoWDocumentHandler.h"
#include "bowText.h"
#include "bowDocument.h"
#include "bowToken.h"
#include "bowTerm.h"
#include "bowNamedEntity.h"
#include "BoWRelation.h"
#include "BoWPredicate.h"

#include "common/LimaCommon.h"
#include "common/Data/readwritetools.h"
#include "common/Data/strwstrtools.h"
#include "common/MediaticData/mediaticData.h"

using namespace Lima::Common::MediaticData;

namespace Lima {
namespace Common {
namespace BagOfWords {

#define BOW_VERSION "0.9"


class BoWBinaryReaderPrivate
{
  friend class BoWBinaryReader;

  BoWBinaryReaderPrivate();
  virtual ~BoWBinaryReaderPrivate();


  std::string m_version;
  BoWFileType m_fileType;

  // entity types mapping
  std::map<MediaticData::EntityGroupId,MediaticData::EntityGroupId> m_entityGroupIdMapping;
  std::map<MediaticData::EntityType,MediaticData::EntityType> m_entityTypeMapping;

  // private member functions
  void readComplexTokenParts(std::istream& file,
                             QSharedPointer< BoWComplexToken > token);
  void readNamedEntityProperties(std::istream& file,
                                 QSharedPointer< BoWNamedEntity >  ne);
  QSharedPointer< BoWRelation > readBoWRelation(std::istream& file);
  QSharedPointer< AbstractBoWElement > readBoWToken(std::istream& file);
  void readSimpleToken(std::istream& file,
                       QSharedPointer< BoWToken > token);
  void readPredicate(std::istream& file,
                    QSharedPointer< BoWPredicate > bowPred);

};

BoWBinaryReaderPrivate::BoWBinaryReaderPrivate():
        m_version(""),
        m_fileType(BOWFILE_NOTYPE),
        m_entityGroupIdMapping(),
        m_entityTypeMapping()
{
}

BoWBinaryReaderPrivate::~BoWBinaryReaderPrivate()
{
}

//***********************************************************************
// reader
BoWBinaryReader::BoWBinaryReader():
        m_d(new BoWBinaryReaderPrivate)
{}

BoWBinaryReader::~BoWBinaryReader()
{
  delete m_d;
}

std::string BoWBinaryReader::getFileTypeString() const
{
    switch (m_d->m_fileType) {
    case BOWFILE_NOTYPE:
        return "BOWFILE_NOTYPE";
    case BOWFILE_TEXT:
        return "BOWFILE_TEXT";
    case BOWFILE_DOCUMENT:
        return "BOWFILE_DOCUMENT";
    case BOWFILE_DOCUMENTST:
        return "BOWFILE_DOCUMENTST";
    case BOWFILE_SDOCUMENT:
        return "BOWFILE_SDOCUMENT";
    }
    return "";
}

void BoWBinaryReader::readHeader(std::istream& file)
{
    Misc::readString(file,m_d->m_version);
    if (m_d->m_version != BOW_VERSION) {
        std::ostringstream oss;
        oss << "uncompatible version : file is in version " << m_d->m_version
        << "; current version is " << BOW_VERSION;
        BOWLOGINIT;
        LERROR << oss.str();
        throw LimaException();
    }
    m_d->m_fileType=static_cast<BoWFileType>(Misc::readOneByteInt(file));
    BOWLOGINIT;

    // read entity types
    Common::MediaticData::MediaticData::changeable().
    readEntityTypes(file,m_d->m_entityGroupIdMapping,m_d->m_entityTypeMapping);
   
#ifdef DEBUG_LP
    LDEBUG << "BoWBinaryReader::readHeader type mapping is";
    std::ostringstream oss;
    for (std::map<MediaticData::EntityType,MediaticData::EntityType>::const_iterator
            it=m_d->m_entityTypeMapping.begin(),it_end=m_d->m_entityTypeMapping.end();
            it!=it_end; it++) {
        oss << (*it).first << " -> " << (*it).second << std::endl;
    }
    LDEBUG << oss.str();
#endif
    LDEBUG << "BoWBinaryReader::readHeader end file at: " << file.tellg();

}

void BoWBinaryReader::readBoWText(std::istream& file,
            BoWText& bowText)
{
    uint64_t size=Misc::readCodedInt(file);
#ifdef DEBUG_LP
    BOWLOGINIT;
    LDEBUG << "BoWBinaryReader::readBoWText file at: " << file.tellg();
    LDEBUG << "BoWBinaryReader::readBoWText got size=" << size;
#endif
    Misc::readString(file,bowText.lang);
#ifdef DEBUG_LP
    LDEBUG << "BoWBinaryReader::readBoWText got lang=" << bowText.lang;
    LDEBUG << "BoWBinaryReader::readBoWText file at: " << file.tellg();
#endif
    // need to store the tokens to handle the references in the part
    // list of the complex tokens
    for (uint64_t i(0); i<size; i++)
    {
#ifdef DEBUG_LP
        LDEBUG << "BoWBinaryReader::readBoWText reading token " << i;
#endif
        QSharedPointer< AbstractBoWElement > token = readBoWToken(file);
        if (token)
        {
          BOWLOGINIT;
          LERROR << __FILE__ << __LINE__ << "BoWBinaryReader::readBoWText do not push token. It should be  deleted right now";
//           bowText.push_back(token);
        }
        else
        {
#ifdef DEBUG_LP
          LWARN << "BoWBinaryReader::readBoWText Warning: read null token in file";
#endif
        }
    }
#ifdef DEBUG_LP
  LDEBUG << "BoWBinaryReader::readBoWText DONE";
#endif
}

void BoWBinaryReader::readBoWDocumentBlock(std::istream& file,
                     BoWDocument& document,
                     AbstractBoWDocumentHandler& handler,
                     bool useIterator)
{
    BoWBlocType blocType = static_cast<BoWBlocType>( Misc::readOneByteInt(file) );
#ifdef DEBUG_LP
    BOWLOGINIT;
    LDEBUG << "BoWBinaryReader::readBoWDocumentBlock: read blocType" << blocType;
#endif
    // new format
    switch ( blocType )
    {
    case HIERARCHY_BLOC:
    {
#ifdef DEBUG_LP
        LDEBUG << "HIERARCHY_BLOC";
#endif
        std::string elementName;
        Misc::readStringField(file,elementName);
        handler.openSBoWNode(&document, elementName);
        break;
    }
    case INDEXING_BLOC:
    {
#ifdef DEBUG_LP
        LDEBUG << "INDEXING_BLOC";
#endif
        std::string elementName;
        Misc::readStringField(file,elementName);
        handler.openSBoWIndexingNode(&document, elementName);
        break;
    }
    case BOW_TEXT_BLOC:
    {
#ifdef DEBUG_LP
        LDEBUG << "BOW_TEXT_BLOC";
#endif
        document.clear();
        readBoWText(file,document);
        handler.processSBoWText(&document, useIterator);
        break;
    }
    case NODE_PROPERTIES_BLOC:
    {
#ifdef DEBUG_LP
        LDEBUG << "NODE_PROPERTIES_BLOC";
#endif
        document.Misc::GenericDocumentProperties::read(file);
        handler.processProperties(&document, useIterator);
        break;
    }
    case END_BLOC:
    {
#ifdef DEBUG_LP
        LDEBUG << "END_BLOC";
#endif
        handler.closeSBoWNode();
        break;
    }
    case DOCUMENT_PROPERTIES_BLOC:
    { // do nothing ?
#ifdef DEBUG_LP
        LDEBUG << "DOCUMENT_PROPERTIES_BLOC";
#endif
    }
    case ST_BLOC:
    { // do nothing ?
#ifdef DEBUG_LP
        LDEBUG << "ST_BLOC";
#endif
        break;
    }
    default:;
    }
}

QSharedPointer< AbstractBoWElement > BoWBinaryReader::readBoWToken(std::istream& file)
{
  return m_d->readBoWToken(file);
}

QSharedPointer< AbstractBoWElement > BoWBinaryReaderPrivate::readBoWToken( std::istream& file )
{
  BOWLOGINIT;
  BoWType type=static_cast<BoWType>(Misc::readOneByteInt(file));
  LDEBUG << "BoWBinaryReader::readBoWToken token type is " << type;
  QSharedPointer< AbstractBoWElement > token;
  switch (type)  {
  case BOW_TOKEN: {
      token=QSharedPointer< BoWToken >( new BoWToken);
      readSimpleToken(file, qSharedPointerCast<BoWToken>(token));
      break;
  }
  case BOW_TERM: {
      token=QSharedPointer< BoWTerm >(new BoWTerm);
      //     LDEBUG << "BoWToken: calling read(file) on term";
      readSimpleToken(file,qSharedPointerCast<BoWToken>(token));
      readComplexTokenParts(file,qSharedPointerCast<BoWComplexToken>(token));
      break;
  }
  case BOW_NAMEDENTITY: {
      token=QSharedPointer< BoWNamedEntity >(new BoWNamedEntity);
//         LDEBUG << "BoWToken: calling read(file) on NE";
      readSimpleToken(file,qSharedPointerCast<BoWToken>(token));
      readComplexTokenParts(file,qSharedPointerCast<BoWComplexToken>(token));
      readNamedEntityProperties(file,qSharedPointerCast<BoWNamedEntity>(token));
      break;
  }
  case BOW_PREDICATE:{
      token=QSharedPointer< BoWPredicate >(new BoWPredicate);
      readPredicate(file,qSharedPointerCast<BoWPredicate>(token));
      break;
  }
  default:
    LERROR << "Read error: unknown token type.";
  }
//   if (token != 0) {LDEBUG << "BoWToken: token=" << qSharedPointerCast<BoWToken>(token);}
  return token;
}

void BoWBinaryReader::readSimpleToken(std::istream& file,
                QSharedPointer< BoWToken > token)
{
  m_d->readSimpleToken(file, token);
}

void BoWBinaryReaderPrivate::readSimpleToken(std::istream& file,
                QSharedPointer< BoWToken > token)
{

  LimaString lemma,inflectedForm;
  Misc::readUTF8StringField(file,lemma);
#ifdef DEBUG_LP
  BOWLOGINIT;
  LDEBUG << "BoWBinaryReader::readSimpleToken file at: " << file.tellg();
  LDEBUG << "BoWBinaryReader::readSimpleToken read lemma: " << lemma;
#endif
  Misc::readUTF8StringField(file,inflectedForm);
#ifdef DEBUG_LP
  LDEBUG << "BoWBinaryReader::readSimpleToken read infl: " << inflectedForm;
#endif
  LinguisticCode category;
  uint64_t position,length;
  category=static_cast<LinguisticCode>(Misc::readCodedInt(file));
  position=Misc::readCodedInt(file);
  length=Misc::readCodedInt(file);
  token->setLemma(lemma);
  token->setInflectedForm(inflectedForm);
  token->setCategory(category);
  token->setPosition(position);
  token->setLength(length);
}

void BoWBinaryReader::readPredicate(std::istream& file,
                   QSharedPointer< BoWPredicate > bowPred){
  m_d->readPredicate(file, bowPred);
}

void BoWBinaryReaderPrivate::readPredicate(std::istream& file,
                                    QSharedPointer< BoWPredicate > bowPred){
  BOWLOGINIT;
  LDEBUG << "BoWBinaryReaderPrivate::readPredicate";
  EntityGroupId groupId = static_cast<EntityGroupId>( Misc::readCodedInt(file));
  EntityTypeId typeId = static_cast<EntityTypeId>(Misc::readCodedInt(file));
  Common::MediaticData::EntityType oldType=Common::MediaticData::EntityType(typeId,groupId);
  Common::MediaticData::EntityType predType=m_entityTypeMapping[oldType];

  bowPred->setPosition(Misc::readCodedInt(file));
  bowPred->setLength(Misc::readCodedInt(file));
  uint64_t roleNb = Misc::readCodedInt(file);
  LDEBUG << "BoWBinaryReader::readPredicate Read "<< roleNb<< "roles associated to the predicate";
  
  bowPred->setPredicateType(predType);
  for (uint64_t i = 0; i < roleNb; i++)
  {
    EntityGroupId roleGrId = static_cast<EntityGroupId>(Misc::readCodedInt(file));
    EntityTypeId roleTypId = static_cast<EntityTypeId>(Misc::readCodedInt(file));
    Common::MediaticData::EntityType oldRoleType=Common::MediaticData::EntityType(roleTypId,roleGrId);
    Common::MediaticData::EntityType roleType=m_entityTypeMapping[oldRoleType];
    QSharedPointer< AbstractBoWElement > role = readBoWToken(file);
    bowPred->roles().insert(roleType,role);
  }
}

void BoWBinaryReaderPrivate::readComplexTokenParts(std::istream& file,
    QSharedPointer< Lima::Common::BagOfWords::BoWComplexToken > token)
{
    BOWLOGINIT;
    LDEBUG << "BoWBinaryReader::readComplexTokenParts";
    // read parts
    uint64_t head=Misc::readCodedInt(file);
    token->setHead(head);
    uint64_t nbParts=Misc::readCodedInt(file);
    for (uint64_t i(0); i<nbParts; i++) {
        QSharedPointer< BoWRelation> rel=readBoWRelation(file);
        bool isInList;
        file.read((char*) &(isInList), sizeof(bool));
        LDEBUG << "BoWBinaryReader::readComplexTokenParts";
            QSharedPointer< BoWToken > tok = qSharedPointerDynamicCast<BoWToken>(readBoWToken(file));
            if (tok)
              token->addPart(rel,tok);
    }
}

void BoWBinaryReaderPrivate::readNamedEntityProperties(std::istream& file, QSharedPointer< Lima::Common::BagOfWords::BoWNamedEntity > ne)
{
    BOWLOGINIT;
    EntityGroupId groupId=static_cast<EntityGroupId>(Misc::readCodedInt(file));
    EntityTypeId typeId=static_cast<EntityTypeId>(Misc::readCodedInt(file));
    Common::MediaticData::EntityType oldType=Common::MediaticData::EntityType(typeId,groupId);
    Common::MediaticData::EntityType seType=m_entityTypeMapping[oldType];
    LDEBUG << "BoWBinaryReader::readNamedEntityProperties read type " << oldType << "-->" << seType;
    ne->setNamedEntityType(seType);
    uint64_t nbFeatures=Misc::readCodedInt(file);
    for (uint64_t i(0); i<nbFeatures; i++) {
        std::string attribute;
        LimaString value;
        Misc::readStringField(file,attribute);
        Misc::readUTF8StringField(file,value);
        ne->addFeature(attribute,value);
    }
}

QSharedPointer< BoWRelation > BoWBinaryReaderPrivate::readBoWRelation(std::istream& file)
{
  BOWLOGINIT;
  QSharedPointer< BoWRelation > rel=QSharedPointer< BoWRelation >(0);
  uint64_t hasRelation=Misc::readCodedInt(file);
  LDEBUG << "BoWBinaryReaderPrivate::readBoWRelation: read hasRelation" << hasRelation;
  if (hasRelation) {
      rel = QSharedPointer< BoWRelation >(new BoWRelation());
      rel->read(file);
  }
  return rel;
}

BoWFileType BoWBinaryReader::getFileType() const
{
  return m_d->m_fileType;
}


//***********************************************************************
// writer
class BoWBinaryWriterPrivate
{
  friend class BoWBinaryWriter;

  BoWBinaryWriterPrivate(const QMap< uint64_t,uint64_t >& shiftFrom);
  ~BoWBinaryWriterPrivate();

    // private member functions
  void writeComplexTokenParts(std::ostream& file,
                              const QSharedPointer< BoWComplexToken > token) const;
  void writeNamedEntityProperties(std::ostream& file,
                                  const QSharedPointer< BoWNamedEntity > ne) const;
  void writeBoWRelation(std::ostream& file,
                        QSharedPointer< BoWRelation > rel) const;
  void writeBoWToken(std::ostream& file,
                     const QSharedPointer< AbstractBoWElement > bowToken) const;
  void writeSimpleToken(std::ostream& file,
                        const QSharedPointer< BoWToken > token) const;
  void writePredicate(std::ostream& file,
                        const QSharedPointer< BoWPredicate > predicate) const;
                        
  QMap< uint64_t,uint64_t > m_shiftFrom;
};

BoWBinaryWriterPrivate::BoWBinaryWriterPrivate(const QMap< uint64_t, uint64_t >& shiftFrom) :
    m_shiftFrom(shiftFrom)
{
  BOWLOGINIT;
  LDEBUG << "BoWBinaryWriterPrivate::BoWBinaryWriterPrivate" << shiftFrom.size();
  LDEBUG << "BoWBinaryWriterPrivate::BoWBinaryWriterPrivate" << m_shiftFrom.size();
 
}

BoWBinaryWriterPrivate::~BoWBinaryWriterPrivate()
{}


BoWBinaryWriter::BoWBinaryWriter(const QMap< uint64_t, uint64_t >& shiftFrom) :
    m_d(new BoWBinaryWriterPrivate(shiftFrom))
{
  BOWLOGINIT;
  LDEBUG << "BoWBinaryWriter::BoWBinaryWriter" << m_d->m_shiftFrom.size();
}

BoWBinaryWriter::~BoWBinaryWriter()
{
  delete m_d;
}

void BoWBinaryWriter::writeHeader(std::ostream& file, BoWFileType type) const
{
  BOWLOGINIT;
  LDEBUG << "BoWBinaryWriter::writeHeader version=" << BOW_VERSION << " ; type=" << type;
  Misc::writeString(file,BOW_VERSION);
  Misc::writeOneByteInt(file,type);
  // write entity types
  MediaticData::MediaticData::single().writeEntityTypes(file);
  LDEBUG << "BoWBinaryWriter::writeHeader end on file " << &file << "at: " << file.tellp();
}

void BoWBinaryWriter::writeBoWText(std::ostream& file,
             const BoWText& bowText) const
{
    BOWLOGINIT;
    Misc::writeCodedInt(file,bowText.size());
    Misc::writeString(file,bowText.lang);
    LDEBUG << "BoWBinaryWriter::writeBoWText wrote lang on file"<<&file<<" at: " << file.tellp();
    uint64_t tokenCounter(0);
    // build reverse map to store in file numbers instead of pointers
    for (BoWText::const_iterator it=bowText.begin(),
            it_end=bowText.end(); it!=it_end; it++) {
        writeBoWToken(file,*it);
        tokenCounter++;
    }
}

void BoWBinaryWriter::writeBoWDocument(std::ostream& file,
                 const BoWDocument& doc) const
{
    BOWLOGINIT;
    LERROR << "BoWBinaryWriter: writeBoWDocument non implemented";
    LERROR << "Can not write "<< doc << " into "<< file;
}

void BoWBinaryWriter::writeBoWToken(std::ostream& file,
              const QSharedPointer< AbstractBoWElement > token) const
{
  m_d->writeBoWToken(file, token);
}

void BoWBinaryWriterPrivate::writeBoWToken( std::ostream& file, const QSharedPointer< Lima::Common::BagOfWords::AbstractBoWElement > token ) const
{
  BOWLOGINIT;
  LDEBUG << "BoWBinaryWriter::writeBoWToken token type is " << token->getType() << &file;
  Misc::writeOneByteInt(file,token->getType());
  switch (token->getType()) {
    case BOW_TOKEN: {
        writeSimpleToken(file,qSharedPointerCast<BoWToken>(token));
        break;
    }
    case BOW_TERM: {
        writeSimpleToken(file,qSharedPointerCast<BoWToken>(token));
        writeComplexTokenParts(file,qSharedPointerCast<BoWComplexToken>(token));
        break;
    }
    case BOW_NAMEDENTITY: {
        QSharedPointer< BoWNamedEntity > ne=qSharedPointerCast<BoWNamedEntity>(token);
        writeSimpleToken(file,qSharedPointerCast<BoWToken>(token));
        writeComplexTokenParts(file,ne);
        writeNamedEntityProperties(file,ne);
        break;
    }
    case BOW_PREDICATE:{
        writePredicate(file,qSharedPointerCast<BoWPredicate>(token));
      break;
    }
    default: {
        BOWLOGINIT;
        LERROR << "BoWBinaryWriter: cannot handle BoWType " << token->getType();
        throw LimaException();
    }
    }
}

void BoWBinaryWriter::writeSimpleToken(std::ostream& file,
                 const QSharedPointer< BoWToken > token) const
{
  m_d->writeSimpleToken(file, token);
}

void BoWBinaryWriterPrivate::writeSimpleToken(std::ostream& file,
                 const QSharedPointer< BoWToken > token) const
{
  BOWLOGINIT;
  LDEBUG << "BoWBinaryWriter::writeSimpleToken write lemma: " << &file << token->getLemma();
  Misc::writeUTF8StringField(file,token->getLemma());
  LDEBUG << "BoWBinaryWriter::writeSimpleToken write infl: " << token->getInflectedForm();
  Misc::writeUTF8StringField(file,token->getInflectedForm());
  Misc::writeCodedInt(file,token->getCategory());
  if (m_shiftFrom.empty())
  {
    LDEBUG << "BoWBinaryWriter::writeSimpleToken shiftFrom is empty";
    Misc::writeCodedInt(file,token->getPosition()-1);
  }
  else 
  {
    LDEBUG << "BoWBinaryWriter::writeSimpleToken shiftFrom from" << token->getPosition();
    QMap<uint64_t,uint64_t>::const_iterator it = m_shiftFrom.lowerBound(token->getPosition()-1);
    if (it == m_shiftFrom.constBegin())
    {
      LDEBUG << "BoWBinaryWriter::writeSimpleToken shiftFrom NO shift";
      Misc::writeCodedInt(file,token->getPosition()-1);
    }
    else
    {
      LDEBUG << "BoWBinaryWriter::writeSimpleToken shiftFrom shift by" << (it-1).value();
      Misc::writeCodedInt(file,token->getPosition()+ (it-1).value()-1);
    }
  }
  Misc::writeCodedInt(file,token->getLength());
}

void BoWBinaryWriter::writePredicate(std::ostream& file,
                        const QSharedPointer< BoWPredicate > predicate) const{
  m_d->writePredicate(file, predicate);
}

void BoWBinaryWriterPrivate::writePredicate(std::ostream& file,
                        const QSharedPointer< BoWPredicate > predicate) const{
  BOWLOGINIT;
  MediaticData::EntityType type=predicate->getPredicateType();
  LDEBUG << "BoWBinaryWriter::writePredicate type" << type;
  Misc::writeCodedInt(file,type.getGroupId());
  Misc::writeCodedInt(file,type.getTypeId());
  Misc::writeCodedInt(file,predicate->getPosition()-1);
  Misc::writeCodedInt(file,predicate->getLength());
  const QMultiMap<Common::MediaticData::EntityType, QSharedPointer< AbstractBoWElement > >& pRoles = predicate->roles();
  LDEBUG << "BoWBinaryWriter::writePredicate nb  roles" << pRoles.size();
  Misc::writeCodedInt(file,pRoles.size());
  for (auto it = pRoles.begin(); it != pRoles.end(); it++)
  {
    Misc::writeCodedInt(file,it.key().getGroupId());
    Misc::writeCodedInt(file,it.key().getTypeId());
    writeBoWToken(file,it.value());
  }
}


void BoWBinaryWriterPrivate::writeComplexTokenParts(std::ostream& file,
                       const QSharedPointer< BoWComplexToken > token) const
{
    BOWLOGINIT;
    // write parts
    Misc::writeCodedInt(file,token->getHead());
    const std::deque<BoWComplexToken::Part>& parts=token->getParts();
    LDEBUG << "BoWBinaryWriter::writeComplexTokenParts nb parts is " << parts.size();
    Misc::writeCodedInt(file,parts.size());

    for (uint64_t i(0); i<parts.size(); i++) {
        writeBoWRelation(file,parts[i].get<0>());
        bool isInList=false;
        LDEBUG << "BoWBinaryWriter::writeComplexTokenParts writing";
        file.write((char*) &(isInList), sizeof(bool));
        writeBoWToken(file,parts[i].getBoWToken());
    }
}

void BoWBinaryWriterPrivate::writeNamedEntityProperties(std::ostream& file,
                           const QSharedPointer< BoWNamedEntity > ne) const
{
    BOWLOGINIT;
    MediaticData::EntityType type=ne->getNamedEntityType();
    LDEBUG << "BoWBinaryWriter::writeNamedEntityProperties : write type " << type.getGroupId() << "." << type.getTypeId();
    Misc::writeCodedInt(file,type.getGroupId());
    Misc::writeCodedInt(file,type.getTypeId());
    const std::map<std::string, LimaString>& features=ne->getFeatures();
    Misc::writeCodedInt(file,features.size());
    for (std::map<std::string, LimaString>::const_iterator
            it=features.begin(), it_end=features.end(); it!=it_end; it++) {
        Misc::writeStringField(file,(*it).first);
        Misc::writeUTF8StringField(file,(*it).second);
    }
}

void BoWBinaryWriterPrivate::writeBoWRelation(std::ostream& file,
                 QSharedPointer< BoWRelation > rel) const
{
    BOWLOGINIT;
    uint64_t hasRelation=(rel!=0);
    LDEBUG << "BoWBinaryWriter::writeBoWRelation: write hasRelation" << hasRelation;
    Misc::writeCodedInt(file, hasRelation);
    if (hasRelation) {
        Misc::writeUTF8StringField(file,rel->getRealization());
        Misc::writeCodedInt(file,rel->getType());
        Misc::writeCodedInt(file,rel->getSynType());
    }
}


} // end namespace
} // end namespace
} // end namespace

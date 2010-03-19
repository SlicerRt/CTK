
// Qt includes
#include <QStringList>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlRecord>

#include <QTime>
#include <QDebug>

// ctkDICOM includes
#include "ctkDICOMModel.h"

struct Node;

//------------------------------------------------------------------------------
class ctkDICOMModelPrivate:public qCTKPrivate<ctkDICOMModel>
{
public:
  ctkDICOMModelPrivate();
  ~ctkDICOMModelPrivate();
  void init();

  enum IndexType{
    RootType,
    PatientType,
    StudyType,
    SeriesType,
    ImageType
  };
 
  void fetch(const QModelIndex& index, int limit);
  Node* createNode(int row, int column, const QModelIndex& parent)const;
  Node* nodeFromIndex(const QModelIndex& index)const;
  QVariant value(const QModelIndex& index, int row, int field)const;
  QString  generateQuery(const QString& fields, const QString& table, const QString& conditions = QString())const;
  void updateQueries(Node* node)const;

  Node*        RootNode;
  QSqlDatabase DataBase;
  QStringList  Headers;
  QString      Sort;
};

//------------------------------------------------------------------------------
struct Node
{
  ~Node()
    {
    foreach(Node* node, this->Children)
      {
      delete node;
      }
    this->Children.clear();
    }
  ctkDICOMModelPrivate::IndexType Type;
  Node*     Parent;
  QVector<Node*> Children;
  int       Row;
  int       Column;
  QSqlQuery Query;
  QString   UID;
  int       RowCount;
  bool      End;
  bool      Fetching;
};

//------------------------------------------------------------------------------
ctkDICOMModelPrivate::ctkDICOMModelPrivate()
{
  this->RootNode     = 0;
}

//------------------------------------------------------------------------------
ctkDICOMModelPrivate::~ctkDICOMModelPrivate()
{
  delete this->RootNode;
  this->RootNode = 0;
}

//------------------------------------------------------------------------------
void ctkDICOMModelPrivate::init()
{
  QCTK_P(ctkDICOMModel);
  this->Headers = QStringList() << "Name" << "Age" << "Scan" << "Date" << "Subject ID"
                  << "Number" << "Institution" << "Referrer" << "Performer";
}

//------------------------------------------------------------------------------
Node* ctkDICOMModelPrivate::nodeFromIndex(const QModelIndex& index)const
{
  return index.isValid() ? reinterpret_cast<Node*>(index.internalPointer()) : this->RootNode;
}

//------------------------------------------------------------------------------
Node* ctkDICOMModelPrivate::createNode(int row, int column, const QModelIndex& parent)const
{
  QCTK_P(const ctkDICOMModel);
  
  Node* node = new Node;
  Node* nodeParent = 0;
  if (row == -1 || column == -1)
    {// root node
    node->Type = ctkDICOMModelPrivate::RootType;
    node->Parent = 0;
    }
  else
    {
    nodeParent = this->nodeFromIndex(parent); 
    nodeParent->Children.push_back(node);
    node->Parent = (nodeParent == this->RootNode ? 0: nodeParent);
    node->Type = ctkDICOMModelPrivate::IndexType(nodeParent->Type + 1);
    }
  node->Row = row;
  node->Column = column;
  if (node->Type != ctkDICOMModelPrivate::RootType)
    {
    int field = nodeParent->Query.record().indexOf("UID");
    node->UID = this->value(parent, row, field).toString();
    }
  
  node->RowCount = 0;
  node->End = false;
  node->Fetching = false;

  this->updateQueries(node);
  
  return node;
}

//------------------------------------------------------------------------------
QVariant ctkDICOMModelPrivate::value(const QModelIndex& parent, int row, int column) const
{
  Node* node = this->nodeFromIndex(parent);
  if (row >= node->RowCount)
    {      
    const_cast<ctkDICOMModelPrivate *>(this)->fetch(parent, row + 256);
    }

  if (!node->Query.seek(row)) 
    {
    qDebug() << node->Query.lastError();
    return QVariant();
    }
  return node->Query.value(column);
}

//------------------------------------------------------------------------------
QString ctkDICOMModelPrivate::generateQuery(const QString& fields, const QString& table, const QString& conditions)const
{
  QString res = QString("SELECT ") + fields + QString(" FROM ") + table;
  if (!conditions.isEmpty())
    {
    res += QString(" WHERE ") + conditions;
    }
  if (!this->Sort.isEmpty())
    {
    res += QString(" ORDER BY ") + this->Sort;
    }
  return res;
}

//------------------------------------------------------------------------------
void ctkDICOMModelPrivate::updateQueries(Node* node)const
{
  // are you kidding me, it should be virtualized here :-)
  QString query;
  switch(node->Type)
    {
    default:
      Q_ASSERT(node->Type == ctkDICOMModelPrivate::RootType);
      break;
    case ctkDICOMModelPrivate::RootType:
      //query = QString("SELECT  FROM ");
      query = this->generateQuery("UID as UID, PatientsName as Name, PatientsAge as Age, PatientsBirthDate as Date, PatientID as \"Subject ID\"","Patients");
      break;
    case ctkDICOMModelPrivate::PatientType:
      //query = QString("SELECT  FROM Studies WHERE PatientsUID='%1'").arg(node->UID);
      query = this->generateQuery("StudyInstanceUID as UID, StudyDescription as Name, ModalitiesInStudy as Scan, StudyDate as Date, AccessionNumber as Number, ReferringPhysician as Institution, ReferringPhysician as Referrer, PerformingPysiciansName as Performer", "Studies",QString("PatientsUID='%1'").arg(node->UID));
      break;
    case ctkDICOMModelPrivate::StudyType:
      //query = QString("SELECT SeriesInstanceUID as UID, SeriesDescription as Name, BodyPartExamined as Scan, SeriesDate as Date, AcquisitionNumber as Number FROM Series WHERE StudyInstanceUID='%1'").arg(node->UID);
      query = this->generateQuery("SeriesInstanceUID as UID, SeriesDescription as Name, BodyPartExamined as Scan, SeriesDate as Date, AcquisitionNumber as Number","Series",QString("StudyInstanceUID='%1'").arg(node->UID));
      break;
    case ctkDICOMModelPrivate::SeriesType:
      //query = QString("SELECT Filename as UID, Filename as Name, SeriesInstanceUID as Date FROM Images WHERE SeriesInstanceUID='%1'").arg(node->UID);
      query = this->generateQuery("Filename as UID, Filename as Name, SeriesInstanceUID as Date", "Images", QString("SeriesInstanceUID='%1'").arg(node->UID));
      break;
    case ctkDICOMModelPrivate::ImageType:
      break;
    }
  node->Query = QSqlQuery(query, this->DataBase);
  foreach(Node* child, node->Children)
    {
    this->updateQueries(child);
    }
}

//------------------------------------------------------------------------------
void ctkDICOMModelPrivate::fetch(const QModelIndex& index, int limit)
{
  QCTK_P(ctkDICOMModel);
  Node* node = this->nodeFromIndex(index);
  if (node->End || limit <= node->RowCount || node->Fetching/*|| bottom.column() == -1*/)
    {
    return;
    }
  node->Fetching = true;

  int newRowCount;
  const int oldRowCount = node->RowCount;

  // try to seek directly
  if (node->Query.seek(limit - 1)) 
    {
    newRowCount = limit;
    } 
  else 
    {
    newRowCount = qMax(oldRowCount, 1);
    if (node->Query.seek(newRowCount - 1)) 
      {
      while (node->Query.next())
        {
        ++newRowCount;
        }
      } 
    else 
      {
      // empty or invalid query
      newRowCount = 0;
      }
    node->End = true; // this is the end.
    }
  if (newRowCount > 0 && newRowCount > node->RowCount) 
    {
    p->beginInsertRows(index, node->RowCount, newRowCount - 1);
    node->RowCount = newRowCount;
    node->Fetching = false;
    p->endInsertRows();
    } 
  else 
    {
    node->RowCount = newRowCount;
    node->Fetching = false;
    }
}

//------------------------------------------------------------------------------
ctkDICOMModel::ctkDICOMModel(QObject* parent)
{
  QCTK_INIT_PRIVATE(ctkDICOMModel);
  qctk_d()->init();
}

//------------------------------------------------------------------------------
ctkDICOMModel::~ctkDICOMModel()
{
}

//------------------------------------------------------------------------------
bool ctkDICOMModel::canFetchMore ( const QModelIndex & parent ) const
{
  QCTK_D(const ctkDICOMModel);
  Node* node = d->nodeFromIndex(parent);
  return !node->End;
}

//------------------------------------------------------------------------------
int ctkDICOMModel::columnCount ( const QModelIndex & _parent ) const
{
  QCTK_D(const ctkDICOMModel);
  Q_UNUSED(_parent);
  return d->Headers.size();
}

//------------------------------------------------------------------------------
QVariant ctkDICOMModel::data ( const QModelIndex & index, int role ) const
{
  QCTK_D(const ctkDICOMModel);
  if (role & ~(Qt::DisplayRole | Qt::EditRole))
    {
    return QVariant();
    }
  QModelIndex indexParent = this->parent(index);
  Node* node = d->nodeFromIndex(indexParent);
  Q_ASSERT(node->Row == indexParent.row());
  if (index.row() >= node->RowCount)
    {      
    const_cast<ctkDICOMModelPrivate *>(d)->fetch(index, index.row());
    }
/*
  if (!node->Query.seek(index.row())) 
    {
    qDebug() << node->Query.lastError();
    return QVariant();
    }
    */
  int field = node->Query.record().indexOf(d->Headers[index.column()]);
  if (field < 0)
    {
    return QVariant();
    }
  return d->value(indexParent, index.row(), field);
  //return node->Query.value(field);
}

//------------------------------------------------------------------------------
void ctkDICOMModel::fetchMore ( const QModelIndex & parent )
{
  QCTK_D(ctkDICOMModel);
  Node* node = d->nodeFromIndex(parent);
  d->fetch(parent, qMax(node->RowCount, 0) + 256);
}

//------------------------------------------------------------------------------
Qt::ItemFlags ctkDICOMModel::flags ( const QModelIndex & index ) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

//------------------------------------------------------------------------------
bool ctkDICOMModel::hasChildren ( const QModelIndex & parent ) const
{
  QCTK_D(const ctkDICOMModel);
  Node* node = d->nodeFromIndex(parent);
  return node->RowCount > 0 || (!node->End && node->Query.seek(0));
}

//------------------------------------------------------------------------------
QVariant ctkDICOMModel::headerData(int section, Qt::Orientation orientation, int role)const
{
  QCTK_D(const ctkDICOMModel);
  if (role & ~(Qt::DisplayRole | Qt::EditRole))
    {
    return QVariant();
    }
  if (orientation == Qt::Vertical)
    {
    return section;
    }
  Q_ASSERT(orientation == Qt::Horizontal);
  Q_ASSERT(section < d->Headers.size());
  return d->Headers[section];
}

//------------------------------------------------------------------------------
QModelIndex ctkDICOMModel::index ( int row, int column, const QModelIndex & parent ) const
{
  QCTK_D(const ctkDICOMModel);
  Node* parentNode = d->nodeFromIndex(parent);
  Node* node = 0;
  foreach(Node* tmpNode, parentNode->Children)
    {
    if (tmpNode->Row == row && 
        tmpNode->Column == column)
      {
      node = tmpNode;
      break;
      }
    }
  if (node == 0)
    {
    node = d->createNode(row, column, parent);
    }
  return this->createIndex(row, column, node);
}

//------------------------------------------------------------------------------
QModelIndex ctkDICOMModel::parent ( const QModelIndex & index ) const
{
  QCTK_D(const ctkDICOMModel);
  Node* node = d->nodeFromIndex(index);
  if (node == 0 || node->Parent == 0)
    {
    return QModelIndex();
    }
  return this->createIndex(node->Parent->Row, node->Parent->Column, node->Parent);
}

//------------------------------------------------------------------------------
int ctkDICOMModel::rowCount ( const QModelIndex & parent ) const
{
  QCTK_D(const ctkDICOMModel);
  Node* node = d->nodeFromIndex(parent);
  if (node->RowCount == 0 && node->End)
    {
    const_cast<ctkDICOMModelPrivate*>(d)->fetch(parent, 256);
    }
  return node->RowCount;
}

//------------------------------------------------------------------------------
void ctkDICOMModel::setDatabase(const QSqlDatabase &db)
{
  QCTK_D(ctkDICOMModel);

  this->beginResetModel();
  d->DataBase = db;
  
  delete d->RootNode;
  d->RootNode = 0;

  if (d->DataBase.tables().empty())
    {
    Q_ASSERT(d->DataBase.isOpen());
    return;
    }
    
  d->RootNode = d->createNode(-1, -1, QModelIndex());
  
  this->endResetModel();

  bool hasQuerySize = d->RootNode->Query.driver()->hasFeature(QSqlDriver::QuerySize);
  if (hasQuerySize && d->RootNode->Query.size() > 0) 
    {
    int newRowCount= d->RootNode->Query.size();
    beginInsertRows(QModelIndex(), 0, qMax(0, newRowCount - 1));
    d->RootNode->RowCount = newRowCount;
    d->RootNode->End = true;
    endInsertRows();
    } 
  else
    {
    d->RootNode->RowCount = 0;
    }
  d->fetch(QModelIndex(), 256);
}

//------------------------------------------------------------------------------
void ctkDICOMModel::sort(int column, Qt::SortOrder order)
{
  QCTK_D(ctkDICOMModel);
  emit layoutAboutToBeChanged();
  d->Sort = QString("\"%1\" %2")
    .arg(d->Headers[column])
    .arg(order == Qt::AscendingOrder ? "ASC" : "DESC");
  d->updateQueries(d->RootNode);
  emit layoutChanged();
}

//------------------------------------------------------------------------------
bool ctkDICOMModel::setHeaderData ( int section, Qt::Orientation orientation, const QVariant & value, int role)
{
  QCTK_D(ctkDICOMModel);
  if (role & ~(Qt::DisplayRole | Qt::EditRole))
    {
    return false;
    }
  if (orientation == Qt::Vertical)
    {
    return false;
    }
  if (value.toString() == d->Headers[section])
    {
    return false;
    }
  d->Headers[section] = value.toString();
  emit this->headerDataChanged(orientation, section, section);
  return true;
}
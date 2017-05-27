#include <string.h>
#include "rm.h"

RelationManager* RelationManager::_rm = 0;

RelationManager* RelationManager::instance()
{
    if(!_rm)
        _rm = new RelationManager();

    return _rm;
}

RelationManager::RelationManager()
{

    Attribute attr;    

    attr.name = "table-id";
    attr.type = TypeInt;
    attr.length = INT_SIZE;
    tableAttrs.push_back(attr);
    
    attr.name = "table-name";
    attr.type = TypeVarChar;
    attr.length = 50;
    tableAttrs.push_back(attr);

    attr.name = "file-name";
    attr.type = TypeVarChar;
    attr.length = 50;
    tableAttrs.push_back(attr);


    attr.name = "table-id";
    attr.type = TypeInt;
    attr.length = INT_SIZE;
    columnAttrs.push_back(attr);
    
    attr.name = "column-name";
    attr.type = TypeVarChar;
    attr.length = 50;
    columnAttrs.push_back(attr);

    attr.name = "column-type";
    attr.type = TypeInt;
    attr.length = INT_SIZE;
    columnAttrs.push_back(attr);

    attr.name = "column-length";
    attr.type = TypeInt;
    attr.length = INT_SIZE;
    columnAttrs.push_back(attr);

    attr.name = "column-position";
    attr.type = TypeInt;
    attr.length = INT_SIZE;
    columnAttrs.push_back(attr);
    
}

RelationManager::~RelationManager()
{
}

RC RelationManager::createCatalog()
{
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
    RC rc;
    

    if((rc = newTable("TABLES", 1))){
        return rc;
    }

    if((rc = newTable("COLUMNS", 2))){
        return rc;
    }

    if((rc = newColumns(1, tableAttrs))){
        return rc;
    }   

    if((rc = newColumns(2, columnAttrs))){
        return rc;
    }   

    return SUCCESS;
}

RC RelationManager::deleteCatalog()
{
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();

    RC rc;

    if((rc = rbfm->destroyFile("TABLES"))){
        return rc;
    }

    if((rc = rbfm->destroyFile("COLUMNS"))){
        return rc;
    }

    return SUCCESS;
}

RC RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs)
{
    RC rc;
    int32_t id = 0;
    if((rc = getNextTableID(id))){
        return rc;
    }

    if((rc = newTable(tableName, id))){
        return rc;
    }
    if((rc = newColumns(id, attrs))){
        return rc;
    } 
    return SUCCESS;
}

RC RelationManager::newTable(const string tableName, int32_t id){    
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
    int32_t name_len = tableName.size();

    FileHandle fileHandle;
    RC rc;
    RID rid;

    if((rc = rbfm->createFile(tableName))){
        return rc;
    }

    if((rc = rbfm->openFile("TABLES", fileHandle))){
        return rc;
    }

    void *data = malloc (200);
    unsigned offset = 0;
    

    char null = 0;
    // null indicator
    memcpy((char*) data + offset, &null, 1);
    offset += 1;

    // table-id
    memcpy((char*) data + offset, &id, INT_SIZE);
    offset += INT_SIZE;
    
    // table name
    memcpy((char*) data + offset, &name_len, INT_SIZE);
    offset += INT_SIZE;
    memcpy((char*) data + offset, tableName.c_str(), name_len);
    offset += name_len;

    // file name
    memcpy((char*) data + offset, &name_len, INT_SIZE);
    offset += INT_SIZE;
    memcpy((char*) data + offset, tableName.c_str(), name_len);
    offset += name_len; 

    rc = rbfm->insertRecord(fileHandle, tableAttrs, data, rid);

    //rbfm->printRecord(tableAttrs, data);

    rbfm->closeFile(fileHandle);
    free (data);
    return rc;
}


RC RelationManager::newColumns(int32_t id, const vector<Attribute> &attrs){
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
        
    RC rc;
    FileHandle fileHandle;
    if (rbfm->openFile("COLUMNS", fileHandle)){
        return rc;
    }

    void *data = malloc(200);

    RID rid;
    for (unsigned i = 0; i < attrs.size(); i++)
    {
        unsigned offset = 0;

        char null = 0;
        // null indicator
        memcpy((char*) data + offset, &null, 1);
        offset += 1;

        // table-id
        memcpy((char*) data + offset, &id, INT_SIZE);
        offset += INT_SIZE;
        
        // column-name
        int name_len = attrs[i].name.length();
        memcpy((char*) data + offset, &name_len, INT_SIZE);
        offset += INT_SIZE;
        memcpy((char*) data + offset, attrs[i].name.c_str(), name_len);
        offset += name_len;

        // column-type
        memcpy((char*) data + offset, &(attrs[i].type), INT_SIZE);
        offset += INT_SIZE;

        // column-length
        memcpy((char*) data + offset, &(attrs[i].length), INT_SIZE);
        offset += INT_SIZE;

        // column-position
        int pos = i+1;
        memcpy((char*) data + offset, &pos, INT_SIZE);
        offset += INT_SIZE;

        rc = rbfm->insertRecord(fileHandle, columnAttrs, data, rid);
        
        if (rc) return rc;
    }

    rbfm->closeFile(fileHandle);
    free(data);
    return SUCCESS;
}

RC RelationManager::getNextTableID(int32_t &id)
{
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
    FileHandle fileHandle;
    RC rc;
    RID rid;

    rc = rbfm->openFile("TABLES", fileHandle);
    if (rc)
        return rc;

    vector<string> projection;
    projection.push_back("table-id");

    RBFM_ScanIterator iter;
    rc = rbfm->scan(fileHandle, tableAttrs, "table-id", NO_OP, NULL, projection, iter);

    
    void *data = malloc (1 + INT_SIZE);
    int32_t max_table_id = 0;
    int32_t now_id = 0;
    while ((rc = iter.getNextRecord(rid, data)) == (SUCCESS))
    {
        // Parse out the table id, compare it with the current max
        memcpy(&now_id, (char *) data + 1, 4);
        max_table_id = max_table_id > now_id ? max_table_id : now_id;
    }


    free(data);
    id = max_table_id + 1;
    rbfm->closeFile(fileHandle);
    iter.close();
    return SUCCESS;

}

RC RelationManager::deleteTable(const string &tableName)
{
    return -1;
}

RC RelationManager::getTableID(const string name, int32_t &id, RID &rid)
{
    FileHandle fileHandle;
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
    RC rc = rbfm->openFile("TABLES", fileHandle);
    if (rc) return rc;

    vector<string> projection;
    projection.push_back("table-id");

    RBFM_ScanIterator iter;
    int len = name.length();
    char * buff = (char *) malloc(4 + len);
    memcpy(buff, &len, 4);
    memcpy(buff + 4, name.c_str(), len);

    rc = rbfm->scan(fileHandle, tableAttrs, "table-name", EQ_OP, buff, projection, iter);
    void *data = malloc (1 + 4 + len);
    
    if ((rc = iter.getNextRecord(rid, data)) ){
        return rc;
    }

    memcpy(&id, (char *)data+1, 4);
    free(buff);
    free(data);
    iter.close();
    rbfm->closeFile(fileHandle);
    return SUCCESS;
}


RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{
    int32_t id;
    RID rid;
    RC rc;
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
    getTableID(tableName, id, rid);
    
    RBFM_ScanIterator iter;
    vector<string> projection;
    projection.push_back("column-name");
    projection.push_back("column-type");
    projection.push_back("column-length");
    projection.push_back("column-position");

    FileHandle fileHandle;
    rc = rbfm->openFile("COLUMNS", fileHandle);
    rc = rbfm->scan(fileHandle, columnAttrs, "table-id", EQ_OP, &id, projection, iter);

    void *data = malloc(200);
    while ((rc = iter.getNextRecord(rid, data)) == SUCCESS){
        Attribute attr; 
        char null;
        int32_t offset = 0;
        memcpy(&null, data, 1);
        offset += 1;

        int32_t len;
        memcpy(&len, (char*) data + offset, 4);
        offset += 4;
        char name[len + 1];
        name[len] = '\0';
        memcpy(name, (char*) data + offset, len);
        offset += len;
        attr.name = name;

        int32_t type;
        memcpy(&type, (char*) data + offset, 4);
        offset += 4;
        attr.type = (AttrType)type;

        int32_t length;
        memcpy(&length, (char*) data + offset, 4);
        offset += 4;
        attr.length = length;

        attrs.push_back(attr);
    }
    
    return SUCCESS;
}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{
    return -1;
}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
    return -1;
}

RC RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid)
{
    return -1;
}

RC RelationManager::readTuple(const string &tableName, const RID &rid, void *data)
{
    return -1;
}

RC RelationManager::printTuple(const vector<Attribute> &attrs, const void *data)
{
	return -1;
}

RC RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data)
{
    return -1;
}

RC RelationManager::scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  
      const void *value,                    
      const vector<string> &attributeNames,
      RM_ScanIterator &rm_ScanIterator)
{
    return -1;
}




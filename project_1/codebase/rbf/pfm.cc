#include "pfm.h"

PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}


PagedFileManager::PagedFileManager()
{
}


PagedFileManager::~PagedFileManager()
{
}


RC PagedFileManager::createFile(const string &fileName)
{
    // modified
    if(ifstream(fileName.c_str())){
        return -1;
    }
    ofstream pFileWrite;   
    pFileWrite.open(fileName.c_str());  
    pFileWrite.close();
    return 0;
}


RC PagedFileManager::destroyFile(const string &fileName)
{
    // modified
    if(!ifstream(fileName.c_str())){
        return -1;
    }
    remove(fileName.c_str());
    return 0;
}


RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
    // modified
    fileHandle.pFile.open(fileName.c_str(), ios::in | ios::out | ios::binary);
    if (!fileHandle.pFile.is_open()) { 
        // file opening failed
        return -1;
    }
    return 0;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
    // modified
   fileHandle.pFile.close();
   return 0;
}


FileHandle::FileHandle()
{
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
    curPage = -1;
}


FileHandle::~FileHandle()
{
}


RC FileHandle::readPage(PageNum pageNum, void *data)
{
    // modified
    readPageCounter += 1;
    pFile.seekg(pageNum*PAGE_SIZE,ios::beg);
    pFile.read((char*)data,PAGE_SIZE); 
    curPage = pageNum;
    return 0;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
    // modified
    pFile.seekp(pageNum*PAGE_SIZE,ios::beg); 
    pFile.write((char*)data,PAGE_SIZE); 
    pFile.flush();
    writePageCounter += 1;
    curPage = pageNum;
    return 0;
}


RC FileHandle::appendPage(const void *data)
{
    // modified
    pFile.seekp(0, ios::end);
    pFile.write((char*)data, PAGE_SIZE);
    pFile.flush();
    appendPageCounter += 1;
    return 0;
}


unsigned FileHandle::getNumberOfPages()
{
    // modified
    pFile.seekg(0, ios::beg);
    streampos fsize = pFile.tellg();
    pFile.seekg(0, ios::end);
    fsize = pFile.tellg() - fsize;
    int count = fsize/PAGE_SIZE;
       
    return count;
}


RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
    return -1;
}

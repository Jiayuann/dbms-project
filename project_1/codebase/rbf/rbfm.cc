#include "rbfm.h"
#include <math.h>
#include <string.h>


RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
}

RecordBasedFileManager::~RecordBasedFileManager()
{
}

RC RecordBasedFileManager::createFile(const string &fileName) {
    PagedFileManager *pfm = PagedFileManager::instance();
    return pfm->createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
    PagedFileManager *pfm = PagedFileManager::instance();
    return pfm->destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
    PagedFileManager *pfm = PagedFileManager::instance();
    return pfm->openFile(fileName, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    PagedFileManager *pfm = PagedFileManager::instance();
    return pfm->closeFile(fileHandle);
}

/**********************************************
*************  Some helper functions   ********
**********************************************/

// assume sizeof(short)==2, which should be true for 64 bit
typedef unsigned short uint16;

// pack offset and length into one struct
// max of uint16 65535, which is large than PAGE_SIZE
struct OffAndLen{
    uint16 off;
    uint16 len;
};

#define NullBit(nulls, i) (nulls[i/CHAR_BIT] & (1 << (CHAR_BIT - (i%CHAR_BIT) - 1)))

#define ptrFreeInPage(page) ((unsigned int *)((char *)page + PAGE_SIZE - sizeof(unsigned int)))
#define ptrNumInPage(page)  ((unsigned int *)((char *)page + PAGE_SIZE - sizeof(unsigned int)*2))
#define nthSlotsInPage(page, n) ((struct OffAndLen *)((struct OffAndLen *)ptrNumInPage(page) - (n+1)))


// approximate size of a record, only used for malloc
unsigned int sizeRecord(const vector<Attribute> &recordDescriptor){
    unsigned int r = 0;
    for(auto &attr : recordDescriptor){
        r += attr.length;
        r += 4;
    }
    return r;
}


// This is dirty part. In input data, VARCHAR is inlined, which makes it impossible for O(1) access for arbitrary field.
// We used variable-length with directory format, with inlining fixed-size fields.
// So data need to be converted upon insert/read
unsigned int convertFormat(const vector<Attribute> &recordDescriptor, const void *src, void *dst){
    /*
    #define COMPILE_TIME_ASSERT(pred) switch(0){case 0:case pred:;}
    COMPILE_TIME_ASSERT(sizeof(offlen) == 4);
    COMPILE_TIME_ASSERT(sizeof(int) == 4);
    COMPILE_TIME_ASSERT(sizeof(float) == 4);
    */

    int fieldCount = recordDescriptor.size();
    unsigned int nullSize = ceil((double) fieldCount / CHAR_BIT);
    const unsigned char *nullsIndicator = (const unsigned char *) src;

    memcpy(dst, src, nullSize);
    unsigned int readOffset = nullSize;

    char *inlined = (char *)dst + nullSize;
    uint16 writeOffset = nullSize + 4 * fieldCount; 
    struct OffAndLen offlen;

    for(int i=0; i<fieldCount; i++){
        auto &attr = recordDescriptor[i];
        if(!NullBit(nullsIndicator, i)){
            if(attr.type == TypeVarChar){
                // VARCHAR
                offlen.off = writeOffset;
                offlen.len = *(int *)((char*)src + readOffset);
                memcpy(inlined + 4*i, &offlen, 4);
                readOffset += sizeof(int);

                memcpy((char *)dst + writeOffset, (char*)src + readOffset, offlen.len);
                writeOffset += offlen.len;
                readOffset += offlen.len;
            }
            else{
                //INT or REAL
                memcpy(inlined + 4*i, (char*)src + readOffset, attr.length);
                readOffset += attr.length;
            }
        }
    }

    return writeOffset;
}

unsigned int convertFormatBack(const vector<Attribute> &recordDescriptor, const void *src, void *dst){
    int fieldCount = recordDescriptor.size();
    unsigned int nullSize = ceil((double) fieldCount / CHAR_BIT);
    memcpy(dst, src, nullSize);
    const unsigned char *nullsIndicator = (const unsigned char *) src;
    unsigned int writeOffset = nullSize;
    struct OffAndLen *offlen;


    for(int i=0; i<fieldCount; i++){
        auto &attr = recordDescriptor[i];
        if(!NullBit(nullsIndicator, i)){
            if(attr.type == TypeVarChar){
                offlen = (struct OffAndLen *)((char *)src + 4*i + nullSize);
                //memcpy(&offlen, (char *)src + 4*i + nullSize, sizeof(offlen));
                *(int *) ((char *)dst + writeOffset) = offlen->len;
                writeOffset += sizeof(struct OffAndLen);
                memcpy((char *)dst + writeOffset, (char*)src + offlen->off, offlen->len);
                writeOffset += offlen->len;
            }
            else{
                memcpy((char *)dst + writeOffset, (char*)src + 4*i + nullSize, attr.length);
                writeOffset += attr.length;
            }
        }    
    }

    return writeOffset;
}

// get the free space in a page
unsigned int freeSpace(char *page){
    int N = *ptrNumInPage(page);
    int offFree = *ptrFreeInPage(page) ;
    return PAGE_SIZE - offFree - N*sizeof(struct OffAndLen) - 2*sizeof(unsigned int);
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
    int fieldCount = recordDescriptor.size();
    unsigned int nullSize = ceil((double) fieldCount / CHAR_BIT); 

    char *buffer = (char *) malloc(nullSize + sizeRecord(recordDescriptor));
    int len = convertFormat(recordDescriptor, data, buffer);
    //int len = dataLength(recordDescriptor, data);

    char *pageBuffer = (char *) malloc(PAGE_SIZE);
    memset(pageBuffer, 0, PAGE_SIZE);
    unsigned int offFree;
    unsigned int N;
    unsigned int pid;

    // if 0 page (newly created)
    if(fileHandle.curPage < 0){
        fileHandle.appendPage(pageBuffer);
        pid = 0;
    }
    else{
        fileHandle.readPage(fileHandle.curPage, pageBuffer);
        if(freeSpace(pageBuffer) > len + sizeof(struct OffAndLen)){
            pid = fileHandle.curPage;
        }
        else{
            unsigned int nPage = fileHandle.getNumberOfPages();
            for(pid = 0; pid < nPage; pid++){
                if(freeSpace(pageBuffer) > len + sizeof(struct OffAndLen)){
                    break;
                }
            }
            if(pid >= nPage){
                memset(pageBuffer, 0, PAGE_SIZE);
                fileHandle.appendPage(pageBuffer);
                pid = nPage;
            }
        }
    }
    fileHandle.curPage = pid;

    // read directory of the page
    offFree = *ptrFreeInPage(pageBuffer);
    N = *ptrNumInPage(pageBuffer);

    // write into memory buffer
    memcpy(pageBuffer + offFree, buffer, len);
    nthSlotsInPage(pageBuffer, N)->off = offFree;
    nthSlotsInPage(pageBuffer, N)->len = len;
    *ptrNumInPage(pageBuffer) =  *ptrNumInPage(pageBuffer) + 1;
    *ptrFreeInPage(pageBuffer) = offFree + len;

    rid.slotNum = N;
    rid.pageNum = pid;
    
    RC r = fileHandle.writePage(pid, pageBuffer);
    free(pageBuffer);
    free(buffer);
    return r;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
    void *pageBuffer = malloc(PAGE_SIZE);
    fileHandle.readPage(rid.pageNum, pageBuffer);
    struct OffAndLen *offlen = nthSlotsInPage(pageBuffer, rid.slotNum);
    //memcpy(data, pageBuffer + offlen->off, offlen->len);
    convertFormatBack(recordDescriptor, (char *)pageBuffer + offlen->off, data);
    free(pageBuffer);
    return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
    //modified
    int offSet = 0;
    int index = 1;
     const unsigned char *nullsIndicator = (const unsigned char *) data;
    int fieldCount = recordDescriptor.size();
    bool nullBit = false;
    int nullFieldsIndicatorActualSize = ceil((double) fieldCount/ CHAR_BIT);
    offSet += nullFieldsIndicatorActualSize;

    for(Attribute attr:recordDescriptor){
    //for(int i=0; i<recordDescriptor.size(); i++){
        //auto &attr = recordDescriptor[i];
        nullBit = NullBit(nullsIndicator, index);
        index += 1;
        if (!nullBit) {
                if(attr.type == TypeVarChar){
                    int nameLength = ((int*)((char *)data + offSet))[0];
                    offSet += sizeof(int);
                    cout << attr.name << ": " << string((char *)data + offSet, nameLength) <<" ";              
                    offSet += nameLength;
                }else if(attr.type == TypeInt){
                    cout << attr.name << ": " << ((int *)((char *)data + offSet))[0] <<" ";
                    offSet += sizeof(int);
                }else{
                    cout<<attr.name<<": "<<((float *)((char *)data + offSet))[0] <<" ";
                    offSet += sizeof(float);
                }   
        }else{
            cout<<attr.name<<": "<<"NULL"<<" ";
        }
    }
    cout<<endl;
    return 0;
}

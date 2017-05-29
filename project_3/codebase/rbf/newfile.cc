RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute &recordDescriptor, const void *data, const RID &rid){
    void *pageData = malloc(PAGE_SIZE);
    if(fileHandle.readPage(rid.pageNum, pageData)){
        free(pageData);
        return RBFM_READ_FAILED;
    }
    SlotDirectoryHeader slotHeader = getSlotDirectoryHeader(pageData);
    if(slotHeader.recordEntriesNumber <= rid.slotNum){
        free(pageData);
        return RBFM_SLOT_DN_EXIST;
    }
    SlotDirectoryRecordEntry recordEntry = getSlotRecordEntry(pageData, rid.slotNum);
    s = getSlotStatus(recordEntry);
    switch(status){
        case DEAD:
            free(pageData);
            return RBFM_AFTER_DEL;
            
    }



}
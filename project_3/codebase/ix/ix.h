#ifndef _ix_h_
#define _ix_h_

#include <vector>
#include <string>
#include <stdlib.h>

#include "../rbf/rbfm.h"

# define IX_EOF (-1)  // end of the index scan
const int NODE_FREE = PAGE_SIZE - sizeof(int);
const int NODE_RIGHT = PAGE_SIZE - ((sizeof(int)) * 2);
const int NODE_TYPE = PAGE_SIZE - ((sizeof(int)) * 3);
const int RID_SIZE = 2 * sizeof(int);
const int SPLIT_THRESHOLD = PAGE_SIZE / 2;
const int DEFAULT_FREE = PAGE_SIZE - (sizeof(int) * 3);

typedef enum{
    TypeNode = 10,
    TypeLeaf = 11,
    TypeRoot = 12,
}NodeType;

class IX_ScanIterator;
class IXFileHandle;

class IndexManager {

    public:
        static IndexManager* instance();

        // Create an index file.
        RC createFile(const string &fileName);

        // Delete an index file.
        RC destroyFile(const string &fileName);

        // Open an index and return an ixfileHandle.
        RC openFile(const string &fileName, IXFileHandle &ixfileHandle);

        // Close an ixfileHandle for an index.
        RC closeFile(IXFileHandle &ixfileHandle);

        // Insert an entry into the given index that is indicated by the given ixfileHandle.
        RC insertEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid);

        // Delete an entry from the given index that is indicated by the given ixfileHandle.
        RC deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid);

        // Initialize and IX_ScanIterator to support a range search
        RC scan(IXFileHandle &ixfileHandle,
                const Attribute &attribute,
                const void *lowKey,
                const void *highKey,
                bool lowKeyInclusive,
                bool highKeyInclusive,
                IX_ScanIterator &ix_ScanIterator);

        // Print the B+ tree in pre-order (in a JSON record format)
        void printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) const;

        /***************************************************************************/
        // Split the child into 2 seperate nodes
        RC splitChild(void *child, void *parent,const Attribute &attribute, IXFileHandle &ixFileHandle, const void *key,int &childPageNum, int &parentPageNum);

        // Get the following node
        RC getNextNodeByKey(void *&child, void *&parent, const Attribute &attribute, IXFileHandle &ixFileHandle,const void *key, int &leftPageNum, int &parentPageNum);

        // Check if the page has enough space
        bool hasEnoughSpace(void *data, const Attribute &attribute);

        // Insert the directory key<key, nextPointer> into a non-leaf node
        RC insertDirector(void *node, const void *key,const Attribute &attribute,int nextPageNum,IXFileHandle &ixFileHandle);
        
        // Return a director at a given offset
        RC getDirectorAtOffset(int &offset, void* node, int &leftPointer, int &rightPointer, void* key, const Attribute &attribute);

        // Insert a key<key, RID> into a leaf
        RC insertIntoLeaf(IXFileHandle &ixFileHandle, void *child, const void *key, const Attribute &attribute, const RID &rid);

        // Delete a key<key, RID> from a leaf
        RC deleteFromLeaf(IXFileHandle &ixFileHandle, void *child, const void *key, const Attribute &attribute, const RID &rid);

        // Compare two keys
        int compareKeys(const void *key1, const void *key2, const Attribute attribute);

        // Get the next offset in a leaf node
        static int getNextKeyOffset(int RIDnumOffset, void *node);

        // Get the number of RIDs in a <key, RID> entry
        static int getNumberOfRids(void *node, int RIDnumOffset);

        // Get the length of a key
        int getKeyLength(const void *key, Attribute attr);

        // Creates an initial key on the insert of a leaf node
        int createNewLeafEntry(void *&data, const void *key, const Attribute &attribute, const RID &rid);

        // Print all nodes of the contents
        RC printNode(void *node, IXFileHandle &ixFileHandle, const Attribute &attribute, int depth, ofstream &myfile) const;

        // Get all keys in a leaf node
        RC getKeysInLeaf(IXFileHandle &ixFileHandle, void *node, const Attribute &attribute, vector<string> &keys) const;

        // Get all keys in a non-leaf node
        RC getKeysInNonLeaf(IXFileHandle &ixFileHandle, void *node, const Attribute &attribute, vector<string> &keys, vector<int> &pages) const;

    protected:
        IndexManager();
        ~IndexManager();

    private:
        static IndexManager *_index_manager;
        PagedFileManager *pfm;
};


class IX_ScanIterator {
    public:

		// Constructor
        IX_ScanIterator();

        // Destructor
        ~IX_ScanIterator();

        // Get next matching entry
        RC getNextEntry(RID &rid, void *key);

        // Terminate index scan
        RC close();

        /***************************************************************************/
        // Get set methods
        void setHandle(IXFileHandle &ixfileHandle) { ixFileHandle = &ixfileHandle; };
        void setAttribute(const Attribute &attr) { attribute = &attr; };
        void setLowKeyValues(const void *lowKey, bool lowKeyInclusive, const Attribute &attribute);
        void setHighKeyValues(const void *highKey, bool highKeyInclusive, const Attribute &attribute);
        void setLeafNode(void *node) { memcpy((char *) leafNode, (char *) node, PAGE_SIZE); };
        void setType(void (*f)(void*&, RID&, void*, int&)) { getKey = f; };
        void setFunc(bool (*f)(void*, const void*, const void*, void*, int, bool, bool)) { compareTypeFunc = f; };
        void setLeafOffset(int newOffset) { currentLeafOffset = newOffset; };
        void setKeyRids(vector<RID> rids) { keyRids = rids; };
        void setKeyIndex(int index) { keyIndex = index; };
        int getLeafOffset() { return currentLeafOffset; };
        RID getNextRid();

        // static functions used to extract types
        static void getIntType(void *&type, RID &rid, void *node, int &offset);
        static void getRealType(void *&type, RID &rid, void *node, int &offset);
        static void getVarCharType(void *&type, RID &rid, void *node, int &offset);
        static bool compareInts(void *incomingKey, const void *low, const void *high, void *node, int offset, bool lowInc, bool highInc);
        static bool compareReals(void *incomingKey, const void *low, const void *high, void *node, int offset, bool lowInc, bool highInc);
        static bool compareVarChars(void *incomingKey, const void *low, const void *high, void *node, int offset, bool lowInc, bool highInc);

    private:
        void *leafNode;
        IXFileHandle *ixFileHandle;
        const Attribute *attribute;
        const void *lowKey;
        const void *highKey;
        bool lowKeyInclusive;
        bool highKeyInclusive;
        int currentLeafOffset;
        bool hasBegan; 
        void (*getKey)(void *&, RID&, void*, int&);
        bool (*compareTypeFunc)(void*, const void*, const void*, void*, int, bool, bool);
        vector<RID> keyRids;
        int keyIndex;

};



class IXFileHandle {
    public:

    // variables to keep counter for each operation
    unsigned ixReadPageCounter;
    unsigned ixWritePageCounter;
    unsigned ixAppendPageCounter;

    // Constructor
    IXFileHandle();

    // Destructor
    ~IXFileHandle();

	// Put the current counter values of associated PF FileHandles into variables
	RC collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount);

    /***************************************************************************/
    FileHandle* getHandle() { return handle; };
    void setHandle(FileHandle *h) { handle = h; };
    void setRoot(void *data) { handle->currentPage = data; };
    void setRootPageNum(int pn) { handle->currentPageNum = pn; };
    void setFreeSpace(void *data, int freeSpace) { memcpy((char*) data + NODE_FREE, &freeSpace, sizeof(int)); };
    void setNodeType(void *node, NodeType type);
    void writeNode(int pageNumber, void* data);
    void readNode(int pageNumber, void *data);
    void* getRoot() { return handle->currentPage; };
    int getRootPageNum() { return handle->currentPageNum; };
    RC getNode(int pageNum, void *node) { return getHandle()->readPage(pageNum, node); };
    int getRightPointer(void *node);
    void setRightPointer(void *node, int rightPageNum);
    int initializeNewNode(void *data, NodeType type); // Initializes a new node, setting it's free space and node type
    int getAvailablePageNumber(); // This helper function will get the first available page

    static int getFreeSpace(void *data);
    static int getFreeSpaceOffset(int freeSpace){ return (DEFAULT_FREE - freeSpace);}
    static NodeType getNodeType(void *node);
    static bool isLeftNodeNull(void *node, int offset, int &childPageNum);
    static bool isRightNodeNull(void *node, const Attribute &attribute, int offset, int &childPageNum);

    private:
        FileHandle *handle;
        vector<int> freePages;

};

#endif

// ListStream

typedef struct ITCListNodeTag
{
    struct ITCListNodeTag* prev;    ///< Previous node
    struct ITCListNodeTag* next;    ///< Next node
    unsigned char* data;            ///< The data
} ITCListNode;

/**
* List stream is a linked-list, memory-based stream.
*/
typedef struct
{
    ITCStream stream;       ///< Base stream definition
    ITCListNode* rootnode;  ///< Root linked-list node
    int datasize;           ///< Data size
    ITCListNode* readnode;  ///< Current reading node
    int readindex;          ///< Current reading node index
    int readpos;            ///< Read position
    ITCListNode* writenode; ///< Current writting node
    int writeindex;         ///< Current writting node index
    int writepos;           ///< Written position
} ITCListStream;

/**
* Opens a list stream.
*
* @param lstream Pointer referring to the list stream.
* @param datasize Data size per node.
* @return Returns 0 if the array stream was successfully opened. A return non-zero value indicates an error.
*/
int itcListStreamOpen(ITCListStream* lstream, int datasize);

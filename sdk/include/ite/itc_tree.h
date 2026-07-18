/** @defgroup itc_tree Tree
 *  @{
 */

/**
 * Tree node definition.
 */
typedef struct ITCTreeTag
{
    struct ITCTreeTag* parent;  ///< Parent node
    struct ITCTreeTag* sibling; ///< Sibling node
    struct ITCTreeTag* child;   ///< Child node
} ITCTree;

/**
 * Pushes a child node to the front of tree.
 *
 * @param tree The tree.
 * @param node The node wiil be pushed.
 */
void itcTreePushFront(ITCTree* tree, void* node);

/**
 * Pushes a child node to the back of tree.
 *
 * @param tree The tree.
 * @param node The node wiil be pushed.
 */
void itcTreePushBack(ITCTree* tree, void* node);

/**
 * Removes a child node from a tree.
 *
 * @param node The node wiil be removed.
 */
void itcTreeRemove(void* node);

void* itcTreeGetChildAtImpl(ITCTree* tree, int index);
int itcTreeGetChildCountImpl(ITCTree* tree);

/**
 * Moves the last child to the first one of a tree.
 *
 * @param tree The tree.
 */
void itcTreeRotateFront(ITCTree* tree);

/**
 * Moves the first child to the last one.
 *
 * @param tree The tree.
 */
void itcTreeRotateBack(ITCTree* tree);

/**
* Swaps 2 nodes of tree.
*
* @param node1 The first node.
* @param node2 The second node.
*/
void itcTreeSwap(void* node1, void* node2);

// Macros to easily use ITCTree structure
/**
 * Gets a child node from a tree at specified index.
 *
 * @param tree The tree.
 * @param index The index.
 * @return The child node.
 */
#define itcTreeGetChildAt(tree, index)      itcTreeGetChildAtImpl((ITCTree*)(tree), (index))

/**
 * Get children count of a tree.
 *
 * @param tree The tree.
 * @return The children count.
 */
#define itcTreeGetChildCount(tree)          itcTreeGetChildCountImpl((ITCTree*)(tree))

/** @} */ // end of itc_tree

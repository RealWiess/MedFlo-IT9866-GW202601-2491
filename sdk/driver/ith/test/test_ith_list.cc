#include <stdint.h>
#include <gtest/gtest.h>
#include <ith/ith_utility.h>

typedef struct Node {
    ITHList list;
    int value;
} Node;

// Helper function used to get the length of the linked list
int getListLength(ITHList* list) {
    int count = 0;
    ITHList* current = list->next;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

// Helper function used to retrieve the values of nodes in the linked list
std::vector<int> getListValues(ITHList* list) {
    std::vector<int> values;
    ITHList* current = list->next;
    while (current != NULL) {
        Node* node = static_cast<Node*>(static_cast<void*>(reinterpret_cast<char*>(current) - offsetof(Node, list)));
        values.push_back(node->value);
        current = current->next;
    }
    return values;
}

void dtor(void * node)
{
    Node * listNode = (Node *)(node);
    listNode->value = 0;
}

TEST(ListTest, PushFront) {
    ITHList list = { NULL, NULL };
    Node node1 = { { NULL, NULL }, 10 };
    Node node2 = { { NULL, NULL }, 20 };

    ithListPushFront(&list, &node1.list);
    ithListPushFront(&list, &node2.list);

    EXPECT_EQ(getListLength(&list), 2);
    EXPECT_EQ(getListValues(&list), std::vector<int>({ 20, 10 }));
}

TEST(ListTest, PushBack) {
    ITHList list = { NULL, NULL };
    Node node1 = { { NULL, NULL }, 10 };
    Node node2 = { { NULL, NULL }, 20 };

    ithListPushBack(&list, &node1.list);
    ithListPushBack(&list, &node2.list);

    EXPECT_EQ(getListLength(&list), 2);
    EXPECT_EQ(getListValues(&list), std::vector<int>({ 10, 20 }));
}

TEST(ListTest, InsertBefore) {
    ITHList list = { NULL, NULL };
    Node node1 = { { NULL, NULL }, 10 };
    Node node2 = { { NULL, NULL }, 20 };
    Node newNode = { { NULL, NULL }, 15 };

    ithListPushBack(&list, &node1.list);
    ithListPushBack(&list, &node2.list);
    ithListInsertBefore(&list, &node2.list, &newNode.list);

    EXPECT_EQ(getListLength(&list), 3);
    EXPECT_EQ(getListValues(&list), std::vector<int>({ 10, 15, 20 }));
}

TEST(ListTest, InsertAfter) {
    ITHList list = { NULL, NULL };
    Node node1 = { { NULL, NULL }, 10 };
    Node node2 = { { NULL, NULL }, 20 };
    Node newNode = { { NULL, NULL }, 15 };

    ithListPushBack(&list, &node1.list);
    ithListPushBack(&list, &node2.list);
    ithListInsertAfter(&list, &node1.list, &newNode.list);

    EXPECT_EQ(getListLength(&list), 3);
    EXPECT_EQ(getListValues(&list), std::vector<int>({ 10, 15, 20 }));
}

TEST(ListTest, RemoveFirstEntryOfAListWithTwoEntries) {
    ITHList list = { NULL, NULL };
    Node node1 = { { NULL, NULL }, 10 };
    Node node2 = { { NULL, NULL }, 20 };

    ithListPushBack(&list, &node1.list);
    ithListPushBack(&list, &node2.list);
    ithListRemove(&list, &node1.list);

    EXPECT_EQ(getListLength(&list), 1);
    EXPECT_EQ(getListValues(&list), std::vector<int>({ 20 }));
}

TEST(ListTest, RemoveFirstEntryOfAListWithOneEntry) {
    ITHList list = { NULL, NULL };
    Node node = { { NULL, NULL }, 10 };

    ithListPushBack(&list, &node.list);
    ithListRemove(&list, &node.list);

    EXPECT_EQ(getListLength(&list), 0);
}

TEST(ListTest, Clear) {
    ITHList list = { NULL, NULL };
    Node node1 = { { NULL, NULL }, 10 };
    Node node2 = { { NULL, NULL }, 20 };

    ithListPushBack(&list, &node1.list);
    ithListPushBack(&list, &node2.list);
    ithListClear(&list, NULL);

    EXPECT_EQ(getListLength(&list), 0);
}

TEST(ListTest, ClearListAndSetAllEntriesValueToZeroViaDestructorFunction) {
    ITHList list = { NULL, NULL };
    Node node1 = { { NULL, NULL }, 10 };
    Node node2 = { { NULL, NULL }, 20 };

    ithListPushBack(&list, &node1.list);
    ithListPushBack(&list, &node2.list);
    ithListClear(&list, &dtor);

    EXPECT_EQ(getListLength(&list), 0);
    EXPECT_EQ(node1.value, 0);
    EXPECT_EQ(node2.value, 0);
}
TEST(ListTest, PushFrontOnEmptyList) {
    ITHList list = { NULL, NULL };
    Node node = { { NULL, NULL }, 10 };

    ithListPushFront(&list, &node.list);

    EXPECT_EQ(getListLength(&list), 1);
    EXPECT_EQ(getListValues(&list), std::vector<int>({ 10 }));
}

TEST(ListTest, PushBackOnEmptyList) {
    ITHList list = { NULL, NULL };
    Node node = { { NULL, NULL }, 10 };

    ithListPushBack(&list, &node.list);

    EXPECT_EQ(getListLength(&list), 1);
    EXPECT_EQ(getListValues(&list), std::vector<int>({ 10 }));
}

TEST(ListTest, InsertBeforeOnOneEntryList) {
    ITHList list = { NULL, NULL };
    Node node1 = { { NULL, NULL }, 10 };
    Node node2 = { { NULL, NULL }, 20 };

    ithListPushBack(&list, &node1.list);
    ithListInsertBefore(&list, &node1.list, &node2.list);

    EXPECT_EQ(getListLength(&list), 2);
    EXPECT_EQ(getListValues(&list), std::vector<int>({ 20, 10 }));
}

TEST(ListTest, InsertAfterOnOneEntryList) {
    ITHList list = { NULL, NULL };
    Node node1 = { { NULL, NULL }, 10 };
    Node node2 = { { NULL, NULL }, 20 };

    ithListPushBack(&list, &node1.list);
    ithListInsertAfter(&list, &node1.list, &node2.list);

    EXPECT_EQ(getListLength(&list), 2);
    EXPECT_EQ(getListValues(&list), std::vector<int>({ 10, 20 }));
}

TEST(ListTest, RemoveFromEmptyList) {
    ITHList list = { NULL, NULL };
    Node node = { { NULL, NULL }, 10 };

    ithListRemove(&list, &node.list);

    EXPECT_EQ(getListLength(&list), 0);
}

TEST(ListTest, ClearEmptyList) {
    ITHList list = { NULL, NULL };

    ithListClear(&list, NULL);

    EXPECT_EQ(getListLength(&list), 0);
}

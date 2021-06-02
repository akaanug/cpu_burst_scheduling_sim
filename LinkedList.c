#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>

// ********** Start of Node ***************//

struct Node {
    struct Node * next;
    int length;
    int t_index;
    struct timeval time;
};

// ********** End of Node *****************//


// ********** Start of LinkedList ***********//
struct LinkedList {
    struct Node * head;
    int size;
};

// LinkedList Constructor:
struct LinkedList* constructList() {
    struct LinkedList* list = malloc( sizeof(struct LinkedList) );
    list->size = 0;
    return list;
}

void add(struct LinkedList* list, int length, int t_index, struct timeval time) {
    if(list->size == 0) { // Add head
        struct Node * head = malloc(sizeof(struct Node));
        head->length = length;
        head->t_index = t_index;
        head->time = time;
        head->next = NULL;
        list->head = head;
        list->size = 1;
        return;
    }

    // Get to the end of the list
    struct Node * curr = list->head;
    for( int i = 0; i < list->size - 1; i++ ) {
        struct Node * next = curr->next;
        curr = next;
    }

    // Append to end.
    struct Node * node = malloc(sizeof(struct Node));
    node->length = length;
    node->t_index = t_index;
    node->time = time;
    curr->next = node;
    list->size++;
}

// Return length and index of the earliest burst of a thread with the given t_index
// Returns -1 if DNE
int getEarliestBurst(struct LinkedList* list, int t_index, int * length) {

    struct Node * curr = list->head;
    for( int i = 0; i < list->size; i++ ) {
        if( curr->t_index == t_index ) {
            *length = curr->length;
            return i;
        }

        curr = curr->next;
    }

    *length = 99999999;
    return -1;
}

// Return priority and index of the earliest of highest priority burst of a thread with the given t_index
// Returns -1 if DNE
int getHighestPriorityBurst(struct LinkedList* list, int t_index, int * priority) {

    struct Node * curr = list->head;
    for( int i = 0; i < list->size; i++ ) {
        if( curr->t_index == t_index ) {
            *priority = curr->t_index;
            return i;
        }

        curr = curr->next;
    }

    *priority = 99999999;
    return -1;
}


struct Node * peak( struct LinkedList* list ) {
    struct Node * curr = list->head;
    for( int i = 0; i < list->size - 1; i++ ) {
        curr = curr->next;
    }

    return curr;
}

void pop( struct LinkedList* list, int *length, int *t_index, struct timeval *time ) {
    if( list->size == 0 ) {
        perror("\nERROR: pop: Size == 0");
        exit(1);
    }

    if( list->size == 1 ) {
        struct Node * curr = list->head;

        *length = curr->length;
        *t_index = curr->t_index;
        *time = curr->time;

        printf("\nPopping: length=%d, t_index=%d\n", *length, *t_index);

        list->size--;
        struct Node * temp = list->head;
        free(temp);
        list->head = NULL;
        return;
    }

    struct Node * temp = list->head;

    *length = temp->length;
    *t_index = temp->t_index;
    *time = temp->time;

    printf("\nPopping: length=%d, t_index=%d, rq size=%d\n", *length, *t_index, list->size);

    list->head = temp->next;
    list->size--;
    free(temp);
}

void remove_item( struct LinkedList* list, int *length, int *t_index, struct timeval *time, int index ) {
    if( index == -1 ) {
        printf("\nItem DNE.");
        return;
    }

    if( (list->size == 0) || (index < 0) || (index >= list->size) ) {
        perror("\nERROR: remove_item\n");
        exit(1);
    }

    if( list->size == 1 ) {
        struct Node * curr = list->head;

        *length = curr->length;
        *t_index = curr->t_index;
        *time = curr->time;

        printf("\nPopping: length=%d, t_index=%d\n", *length, *t_index);

        list->size--;
        list->head = NULL;
        free(curr);
        return;
    }

    if( index == 0 ) {
        struct Node * curr = list->head;
        list->head = list->head->next;
        list->size--;

        *length = curr->length;
        *t_index = curr->t_index;
        *time = curr->time;

        printf("\nPopping: length=%d, t_index=%d\n", *length, *t_index);

        free(curr);
        return;
    }

    struct Node * curr = list->head;
    struct Node * prev;
    for( int i = 0; i < index; i++ ) {
        prev = curr;
        curr = curr->next;
    }

    prev->next = curr->next;

    *length = curr->length;
    *t_index = curr->t_index;
    *time = curr->time;

    printf("\nPopping: length=%d, t_index=%d\n", *length, *t_index);

    list->size--;
    free(curr);
}

bool isEmpty(const struct LinkedList* list) {
    return list->head == NULL;
}

void printList(const struct LinkedList* list) {
    // Get to the end of the list
    struct Node * curr = list->head;
    printf("\nList size = %d", list->size);

    if(isEmpty(list)) {
        printf("\nList is empty\n");
        return;
    }
    printf("\nHead: %d", 0);
    printf(" = t_index: %d, length: %d", curr->t_index, curr->length);
    printf("\n");
    for( int i = 0; i < list->size - 1; i++ ) {
        struct Node * next = curr->next;
        curr = next;
        printf("Node: %d", i + 1);
        printf(" = t_index: %d, length: %d", curr->t_index, curr->length);
        printf("\n");
    }
}


void deallocateList(struct LinkedList* list) {

    if(list->size == 0) {
        free(list);
        return;
    }

    struct Node * curr = list->head;
    for( int i = 0; i < list->size; i++ ) {
        struct Node * next = curr->next;
        free(curr);
        curr = next;
    }

    free(list);
}

// ********** End of LinkedList ***********//


/*
int main() {

    struct timeval currTime;

    struct LinkedList * list = constructList();
    gettimeofday(&currTime, NULL);
    add(list, 500, 1, currTime);
    add(list, 400, 1, currTime);
    add(list, 200, 2, currTime);
    add(list, 300, 3, currTime);
    add(list, 100, 2, currTime);
    add(list, 900, 3, currTime);
    int i, l;
    i = getEarliestBurst(list, 1, &l);
    printf("\nEarliest burst: index=%d, length=%d\n", i, l);

    printList(list);
    remove_item(list, &i, &l, &currTime, i);
    printList(list);

    return 0;


}*/
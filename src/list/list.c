#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

LinkedList *create_list(size_t data_size) {
    LinkedList *list = malloc(sizeof(LinkedList));
    list->head = NULL;
    list->data_size = data_size;
    list->length = 0;
    return list;
}

void append(LinkedList *list, void *element) {
    Node *new_node = malloc(sizeof(Node));
    new_node->data = malloc(list->data_size);
    memcpy(new_node->data, element, list->data_size);
    new_node->next = NULL;

    if (!list->head) {
        list->head = new_node;
    } else {
        Node *temp = list->head;
        while (temp->next) temp = temp->next;
        temp->next = new_node;
    }
    list->length++;
}

void *get_element_at(LinkedList *list, size_t index) {
    if (index >= list->length) return NULL;

    Node *current = list->head;
    for (size_t i = 0; i < index; ++i)
        current = current->next;

    return current->data;
}

int set_element_at(LinkedList *list, size_t index, void *element) {
    if (index >= list->length) return 0;

    Node *current = list->head;
    for (size_t i = 0; i < index; ++i)
        current = current->next;

    memcpy(current->data, element, list->data_size);
    return 1;
}

int remove_at(LinkedList *list, size_t index) {
    if (index >= list->length) return 0;

    Node *temp = list->head;
    Node *prev = NULL;

    for (size_t i = 0; i < index; ++i) {
        prev = temp;
        temp = temp->next;
    }

    if (prev)
        prev->next = temp->next;
    else
        list->head = temp->next;

    free(temp->data);
    free(temp);
    list->length--;
    return 1;
}

size_t list_length(LinkedList *list) {
    return list->length;
}

void print_list(LinkedList *list, void (*print_func)(void *)) {
    Node *current = list->head;
    while (current) {
        print_func(current->data);
        current = current->next;
    }
}

void free_list(LinkedList *list, void (*free_data)(void *)) {
    Node *current = list->head;
    while (current) {
        Node *next = current->next;

        if (free_data)  // Safe to pass NULL if you don't need it
            free_data(current->data);

        free(current);
        current = next;
    }
    free(list);
}
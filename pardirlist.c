/*
 * CSE 420 Project 2
 * Author: Raymond Lawrence
 *
 * TODO:
 * -change input to require <keyword> and <ispar> params
 *      require <keyword> to be a single word
 * add <keyword_frequency> to each node
 *      searches for instances in FILES (folders have value of zero)
 * Add multithreading functionality for ispar = 1
 *
 */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>

/* linked list w/ subroutines */

struct node {
    char *path;
    int level;
    int keyword_frequency;
    struct node *next;
    struct node *prev;
};

struct list {
    struct node *head;
    struct node *tail;
};

//creation subroutines

struct node *create_node(char *path, int level, int keyword_frequency)
{
    struct node *node = malloc(sizeof(struct node));
    if (node == NULL) {
        fprintf(stderr, "%s: couldn't create memory for list; %s\n", "dirlist", strerror(errno));
        exit(-1);
    }
    node->path = strdup(path);
    node->level = level;
    node->keyword_frequency = keyword_frequency;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

struct list *create_list()
{
    struct list *list = malloc(sizeof(struct list));
    if (list == NULL) {
        fprintf(stderr, "%s: couldn't create memory for list; %s\n", "dirlist", strerror(errno));
        exit(-1);
    }
    list->head = NULL;
    list->tail = NULL;
    return list;
}

//frequency helper functions

int seq_search_file(char *file_path, char *keyword)
{
    FILE *fs;
    char buff[1024];
    char *token, *context;
    int frequency;

    fs = fopen(file_path, "r");
    context = NULL;
    frequency = 0;
    
    while (fgets(buff, 1024, fs) != NULL) {
        token = strtok_r(buff, " \t", &context);
        while (token != NULL) {
            if (strcmp(token, keyword) == 0)
                frequency++;
            token = strtok_r(NULL, " ", &context);
        }
    }

    fclose(fs);
    return frequency;
}

int par_search_file()
{
    return 0;   
}

//inserts

void insert_sorted(struct node *node, struct list *list)
{
    if (list->head == NULL && list->tail == NULL) {
        list->head = node;
        list->tail = node;
    } else if (strcmp(node->path, list->head->path) <= 0) {
        list->head->prev = node;
        node->next = list->head;
        list->head = node;
    } else if (strcmp(node->path, list->tail->path) > 0) {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    } else {
        struct node *ptr = list->head;
        while (ptr->next != NULL && strcmp(node->path, ptr->path) > 0)
            ptr = ptr->next;
        node->next = ptr;
        node->prev = ptr->prev;
        ptr->prev->next = node;
        ptr->prev = node;
    }
}

void populate_list(char *path, struct list *list, char *keyword)
{
    static int current_level = 1;
    if (current_level == 1) {
        insert_sorted(create_node(path, current_level, 0), list);
        current_level++;
    }
    DIR *ds = opendir(path);
    char tmp[255];
    struct dirent *d;
    struct stat buf;
    while ((d = readdir(ds)) != NULL) {
        if (d->d_name[0] == '.')    //if hidden file, continue
            continue;
        
        /* create a temporary string containing {PATH}/{DIRECTORY NAME} */
        strcpy(tmp, path);
        strcat(tmp, "/");
        strcat(tmp, d->d_name);
        
        stat(tmp, &buf);            //populate buf with file information
        struct node *new = create_node(tmp, current_level, 0);
        insert_sorted(new, list);
        if (S_ISDIR(buf.st_mode)) {
            current_level++;
            populate_list(tmp, list, keyword);
            current_level--;
        } else {
            new->keyword_frequency = seq_search_file(new->path, keyword);
        }
    }
    closedir(ds);
}

//deletions

void destroy_list(struct list *list)
{
    struct node *curr = list->head, *tmp;
    while (curr != NULL) {
        free(curr->path);
        tmp = curr;
        curr = curr->next;
        free(tmp);
    }
    free(list);
}

//prints

void print_list_to_file(struct list *list, char *filename)
{
    int order;
    struct node *curr = list->head;
    FILE *fs = fopen(filename, "w");
    while (curr != NULL) {
        if (curr->prev != NULL && curr->level == curr->prev->level)
            order++;
        else
            order = 1;
        fprintf(fs, "%d:%d:%d:%s\n", curr->level, order, curr->keyword_frequency, curr->path);
        curr = curr->next;
    }
    fclose(fs);
}

void insertion_sort_by_level_increasing(struct list *list)
{
    struct node *fi = list->head->next, *bi, *tmp;
    int key;
    while (fi != NULL) {
        key = fi->level;
        bi = fi->prev;
        tmp = fi;
        fi = fi->next;
        while (bi != list->head && bi->level > key)
            bi = bi->prev;

        //remove tmp and place it after bi 
        if (tmp->next != NULL)
            tmp->next->prev = tmp->prev;
        tmp->prev->next = tmp->next;
        tmp->next = bi->next;
        tmp->prev = bi;
        if (bi->next != NULL)
            bi->next->prev = tmp;
        bi->next = tmp;
    }
}

/* main */

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "dirlist: usage: dirlist <directory_path> <keyword> <output_file> <ispar>\n");
        return 1;
    }

    char *dirpath = argv[1], *keyword = argv[2], *outfile = argv[3];
    int ispar = atoi(argv[4]);

    if (ispar != 0 && ispar != 1) {
        fprintf(stderr, "dirlist: <ispar> must be 0 or 1");
        return 1;
    }

    struct list *dirlist = create_list();
    populate_list(dirpath, dirlist, keyword);
    insertion_sort_by_level_increasing(dirlist);
    print_list_to_file(dirlist, outfile);    
    destroy_list(dirlist);
    return 0;
}

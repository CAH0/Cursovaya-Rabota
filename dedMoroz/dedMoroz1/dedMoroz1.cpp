#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <locale.h>

#define MAX_GIFTS 100000
#define LINE_LENGTH 256
#define HASH_SIZE 100003

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    int id;
    double latitude;
    double longitude;
    double weight;
} Gift;

typedef struct TreeNode {
    double distance; 
    Gift* gift; 
    struct TreeNode* left;
    struct TreeNode* right;
} TreeNode;

typedef struct HashSet {
    int* table;
    int size;
} HashSet;

HashSet* create_hashset(int size) {
    HashSet* set = (HashSet*)malloc(sizeof(HashSet));
    set->table = (int*)calloc(size, sizeof(int)); 
    set->size = size;
    return set;
}

int hash(int key, int size) {
    return key % size;
}

bool add(HashSet* set, int key) {
    int index = hash(key, set->size);
    if (set->table[index] == 0) { 
        set->table[index] = key;
        return true; 
    }
    return false;
}

bool contains(HashSet* set, int key) {
    int index = hash(key, set->size);
    return set->table[index] != 0; 
}

void free_hashset(HashSet* set) {
    free(set->table);
    free(set);
}

double geo_distance(double lon1, double lat1, double lon2, double lat2) {
    double dlon = (lon2 - lon1) * M_PI / 180.0;
    double dlat = (lat2 - lat1) * M_PI / 180.0;

    lat1 = lat1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;

    double a = sin(dlat / 2) * sin(dlat / 2) +
        cos(lat1) * cos(lat2) *
        sin(dlon / 2) * sin(dlon / 2);
    double c = 2 * asin(sqrt(a));

    double r = 6371; 
    return c * r; 
}

TreeNode* create_node(double distance, Gift* gift) {
    TreeNode* new_node = (TreeNode*)malloc(sizeof(TreeNode));
    new_node->distance = distance;
    new_node->gift = gift; 
    new_node->left = NULL;
    new_node->right = NULL;
    return new_node;
}

TreeNode* insert(TreeNode* root, double distance, Gift* gift) {
    if (root == NULL) {
        return create_node(distance, gift);
    }
    if (distance < root->distance) {
        root->left = insert(root->left, distance, gift);
    }
    else {
        root->right = insert(root->right, distance, gift);
    }
    return root;
}

TreeNode* delete_node(TreeNode* root, int gift_id) {
    if (root == NULL) {
        return NULL; 
    }

    if (gift_id < root->gift->id) {
        root->left = delete_node(root->left, gift_id);
    }
    else if (gift_id > root->gift->id) {
        root->right = delete_node(root->right, gift_id);
    }
    else {
        if (root->left == NULL) {
            TreeNode* temp = root->right;
            free(root);
            return temp;
        }
        else if (root->right == NULL) {
            TreeNode* temp = root->left;
            free(root);
            return temp;
        }

        TreeNode* temp = root->right;
        while (temp && temp->left != NULL) {
            temp = temp->left;
        }

        root->gift = temp->gift;

        root->right = delete_node(root->right, temp->gift->id);
    }
    return root;
}

void free_tree(TreeNode* root) {
    if (root != NULL) {
        free_tree(root->left);
        free_tree(root->right);
        free(root);
    }
}

double traverse_min_branch(TreeNode* root, HashSet* visited_set, double total_weight, int* visited_now, int* visited_count) {
    if (root == NULL) {
        return total_weight; 
    }

    TreeNode** stack = (TreeNode**)malloc(MAX_GIFTS * sizeof(TreeNode*));
    if (stack == NULL) {
        perror("Ошибка выделения памяти для стека");
        return total_weight; 
    }

    int stack_top = -1; 
    TreeNode* current = root; 
    int processed_count = 0; 

    while (current != NULL || stack_top >= 0) {
        while (current != NULL) {
            stack[++stack_top] = current; 
            current = current->left; 
        }
        current = stack[stack_top--]; 

        if (!contains(visited_set, current->gift->id)) {
            if (total_weight + current->gift->weight <= 100) {
                if (add(visited_set, current->gift->id)) {
                    visited_now[*visited_count] = current->gift->id;
                    (*visited_count)++;
                    total_weight += current->gift->weight; 
                }
            }
        }
        current = current->right; 
    }
    free(stack); 
    return total_weight; 
}

int main() {
    setlocale(LC_ALL, "rus");
    FILE* file;
    Gift* gifts = (Gift*)malloc(MAX_GIFTS * sizeof(Gift));
    if (gifts == NULL) {
        perror("Ошибка выделения памяти для массива подарков");
        return EXIT_FAILURE;
    }

    int count = 0;
    char line[LINE_LENGTH];
    file = fopen("gifts.csv", "r");
    if (file == NULL) {
        perror("Ошибка открытия файла");
        free(gifts);
        return EXIT_FAILURE;
    }

    fgets(line, LINE_LENGTH, file);

    while (count < MAX_GIFTS && fgets(line, LINE_LENGTH, file) != NULL) {
        sscanf(line, "%d,%lf,%lf,%lf",
            &gifts[count].id,
            &gifts[count].latitude,
            &gifts[count].longitude,
            &gifts[count].weight);
        count++;
    }

    fclose(file);

    double reference_lat = 60.7603243;
    double reference_lon = 46.3053893;

    TreeNode* root = NULL;
    for (int i = 0; i < count; i++) {
        double distance = geo_distance(reference_lon, reference_lat, gifts[i].longitude, gifts[i].latitude);
        root = insert(root, distance, &gifts[i]); 
    }
    HashSet* visited_set = create_hashset(HASH_SIZE);
    if (visited_set == NULL) {
        perror("Ошибка выделения памяти для хеш-таблицы");
        free_tree(root);
        free(gifts);
        return EXIT_FAILURE;
    }
    FILE* output_file = fopen("answer10.txt", "w");
    if (output_file == NULL) {
        perror("Ошибка открытия файла для записи");
        free_hashset(visited_set);
        free_tree(root);
        free(gifts);
        return EXIT_FAILURE;
    }
    int flag = 0;
    for (int i = 0; i < 20000; i++) {
        int visited_now[MAX_GIFTS] = { 0 };
        int visited_count = 0;
        
        double total_weight = 0;
        if (flag == 0) total_weight = traverse_min_branch(root, visited_set, total_weight, visited_now, &visited_count);
        else  break;
        if (visited_count > 0) {
            fprintf(output_file, "%d", i + 1);
            for (int j = 0; j < visited_count; j++) {
                if (visited_now[j] != 0)
                    fprintf(output_file, ", %d", visited_now[j]);
                else flag = 1;
            }
            fprintf(output_file, "\n");
        }
    }
    printf("Данные успешно сохранены в файл 10answer.txt\n\n");
    fclose(output_file);
    free_hashset(visited_set);
    free_tree(root);
    free(gifts);

    return EXIT_SUCCESS;
}
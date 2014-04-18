// TODO header files
#include "rvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

// A GList list holding all segments
GList* GLOBAL_segment_list = NULL;

// A GHashTable that maps transaction ID to transaction information
GHashTable* GLOBAL_transaction_hashtable = NULL;

/* ------------- UTILITY FUNCTIONS ------------- */

// Enum to support usage of boolean types in C
typedef enum { false, true } bool;

// Utility function for formatting log messages
// TODO: replace printf() statements with utility_log()
void utility_log(const char* message) {
    // Change this to false to disable log messages
    bool enable_log = true;

    if(enable_log) {
        printf("%s", message);
    }
}

// Utility function that dumps/writes the data to disk
void utility_dump_to_disk(rvm_t rvm, const char* segment_name, int truncation_flag) {
    // Handles to files used
    FILE* file_log;
    FILE* file_segment;
    file_log = fopen(rvm.log_path, "r");
    if (file_log == NULL) {
        printf("@utility_dump_to_disk: Error, could not open log file\n");
        abort();
    }

    char ch;
    char seg_data[GLOBAL_MAX_SEGMENT_SIZE];
    char buffer[GLOBAL_MAX_SEGMENT_SIZE + GLOBAL_FILENAME_LIMIT + 10];
    int segment_size;

    while (fgets(buffer, GLOBAL_MAX_SEGMENT_SIZE + GLOBAL_FILENAME_LIMIT + 10, file_log) != NULL) {
        const int limit = 128;
        char current_segment_name[limit];
        char current_segment_size[limit];

        int i = 0; // we will reuse this
        while(buffer[i] != '~') {
            current_segment_name[i] = buffer[i];
            i++;
        }
        current_segment_name[i] = '\0';

        printf("@utility_dump_to_disk: Segment name found in log %s\n", current_segment_name);
        if (!truncation_flag && strcmp(segment_name, current_segment_name)) {
            continue;
        }

        char current_segment_name_full[GLOBAL_FILENAME_LIMIT];
        snprintf(current_segment_name_full, GLOBAL_FILENAME_LIMIT, "%s/%s.seg", rvm.dir_path, current_segment_name);
        file_segment = fopen(current_segment_name_full, "w");
        if(file_segment == NULL){
            printf("@utility_dump_to_disk: Error, could not open segment file\n");
            abort();
        }

        int j = 0;
        i++;
        while(buffer[i] != '~') {
            current_segment_size[j++] = buffer[i];
            i++;
        }
        current_segment_size[j] = '\0';
        segment_size = atoi(current_segment_size);

        j = 0;
        ++i;
        for (;j < segment_size; j++, i++) {
            fputc(buffer[i], file_segment);
        }
        printf("@utility_dump_to_disk: Recovered/Flushed %s commit record to store\n", current_segment_name);
        fclose(file_segment);
    }
    
    fclose(file_log);
    if(truncation_flag) {
        FILE* file_delete_log = fopen(rvm.log_path, "w");
        if(fclose(file_delete_log)) {
            printf("@utility_dump_to_disk: Error, log file truncation failed\n");
        }
        else {
            printf("@utility_dump_to_disk: Successfully truncated log file\n");
        }
    }
}

// Utility function that checks if a file exists
int utility_file_exists(const char* file) {
    struct stat statbuf;
    // @Citation: Inspired by http://lxr.free-electrons.com/source/fs/open.c
    int file_handle = open(file, O_RDONLY);
    int result = !fstat(file_handle, &statbuf);
    close(file_handle);
    return result;
}

// The GList library does not include a find function (equivalant of std::find),
// thus we implement a helper function that finds a segment
seg_t* utility_find_segment(void* segment) {
    GList* iterator = GLOBAL_segment_list;
    while(iterator != NULL) {
        seg_t* new_segment = (seg_t*)iterator->data;
        if(new_segment->mem == segment) {
            return new_segment;
        }
        iterator = iterator->next;
    }
    return NULL;
}

/* ------------- RVM FUNCTIONS ------------- */

rvm_t rvm_init(const char* directory) {
    // Initialize data structure to return later
    rvm_t rvm;

    // Create the log filename with directory name included
    char* log_path = (char*)malloc(GLOBAL_FILENAME_LIMIT);
    snprintf(log_path, GLOBAL_FILENAME_LIMIT, "%s/%s", directory, GLOBAL_LOG_FILE_NAME);

    // Check if directory exists
    if (!utility_file_exists(directory)) {
        if(mkdir(directory, S_IRWXU)) { // returns 0 when true, -1 when failed
            printf("@rvm_init: Error, could not create directory\n");
            exit(EXIT_FAILURE);
        } else {
            printf("@rvm_init: Successfully created directory\n");
        }
        
        if(fopen(log_path, "w") == NULL) {
            printf("@rvm_init: Error, could not create log file\n");
        }
    } else {
        printf("@rvm_init: Existing directory found, reusing\n");
        if(!utility_file_exists(log_path)) {
            printf("@rvm_init: Error, log file not found in existing directory\n");
            abort();
        }
    }

    // Now that the log file is open, set info and clear list
    rvm.dir_path = directory;
    rvm.log_path = log_path;
    GLOBAL_segment_list = NULL;
    printf("@rvm_init: Successfully initialized!\n");
    return rvm;
}

void* rvm_map(rvm_t rvm, const char* segname, int size_to_create) {
    // Declare variables used throughout
    struct stat st;
    int file_handle, result;
    const int path_length = 64;
    char file_path[path_length];
    FILE* file;
    void* buffer;
    snprintf(file_path, path_length, "./%s/%s.seg", rvm.dir_path, segname);
    
    // File already exists on disk
    if(utility_file_exists(file_path)) {
        // Dump current to disk
        utility_dump_to_disk(rvm, segname, 0);
        
        // Read from store and transfer to RAM
        struct stat stat1;
        result = stat(file_path, &stat1);
        unsigned long file_length = stat1.st_size;
        file_handle = open(file_path, O_RDWR, (mode_t) 0600);
        
        // Proceed to one byte before the size_to_create
        // and write a byte to extend
        if(file_length < size_to_create) {
            if(!file_handle) {
                printf("@rvm_map: Error, could not open segment file\n");
                exit(EXIT_FAILURE);
            }

            result = lseek(file_handle, 0, SEEK_SET);
            result = lseek(file_handle, size_to_create - 1, SEEK_SET);
            if(!result) {
                close(file_handle);
                printf("@rvm_map: Error, Unable to seek file\n");
                exit(EXIT_FAILURE);
            }

            result = write(file_handle, "", 1);
            if (!result) {
                printf("@rvm_map: Error, could not write byte\n");
                exit(EXIT_FAILURE);
            }

            struct stat stat2;
            int result = stat(file_path, &stat2);
            file_length = stat2.st_size;
            printf("@rvm_map: Segment %s expanded to new size: %ld\n", segname, file_length);
        }

        printf("@rvm_map: Successfully read segment at %s with size %d\n", file_path, size_to_create);

    } else { // File does not exist
        file_handle = open(file_path, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
        if(!file_handle) {
            printf("@rvm_map: Error, could not create segment file\n");
            exit(EXIT_FAILURE);
        }

        result = lseek(file_handle, size_to_create - 1, SEEK_SET);
        if (!result) {
            close(file_handle);
            printf("@rvm_map: Error, Unable to seek file\n");
            exit(EXIT_FAILURE);
        }

        result = write(file_handle, "", 1);
        if (!result) {
            printf("@rvm_map: Error, could not write byte\n");
            exit(EXIT_FAILURE);
        }
        printf("@rvm_map: Segment %s created with path %s\n", segname, file_path);
    }

    buffer = mmap(0, size_to_create, PROT_READ | PROT_WRITE, MAP_PRIVATE, file_handle, 0);
    if(buffer == MAP_FAILED) {
        printf("@rvm_map: Error, buffer mmap failed\n");
    }

    // Create seg_t object, allocate memory, and populate
    seg_t* segment = (seg_t*)malloc(sizeof(struct seg));
    segment->name = segname;
    segment->size = size_to_create;
    segment->mem = buffer;

    // Add a pointer of this object to the segment list
    GLOBAL_segment_list = g_list_append(GLOBAL_segment_list, segment);
    printf("@rvm_map: Successfully mapped segment %s\n", segname);
    return buffer;
}

void rvm_unmap(rvm_t rvm, void* segbase) {
    // This function is pretty straight forward;
    // check for segment and unmap using munmap()
    seg_t* segment = utility_find_segment(segbase);
    if(segment == NULL) {
        printf("@rvm_unmap: Error, segment not found\n");
        abort();
    }

    if(munmap(segbase, segment->size) == -1) {
        printf("@rvm_unmap: Error, could not munmap segment file\n");
    } else {
        GList* iterator = g_list_find(GLOBAL_segment_list, segment);
        GLOBAL_segment_list = g_list_remove_link(GLOBAL_segment_list, iterator);
        printf("@rvm_unmap: Successfully mapped segment\n");
    }
}

trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases) {
    // Initial variable
    int tid;

    if(GLOBAL_transaction_hashtable == NULL) {
        // Initialize a new hashtable for this transaction
        GLOBAL_transaction_hashtable = g_hash_table_new(g_int_hash, g_int_equal);
        tid = 1;
    } else {
        // Hashtable already exists, get id based on number of elements
        // The GLib hash table implementation does not support length
        // So we create a list from all keys and use its length to set tid
        GList* new_list = g_hash_table_get_keys(GLOBAL_transaction_hashtable);
        tid = g_list_length(new_list) + 1;
    }

    trans_info_t* info = (trans_info_t*)malloc(sizeof(trans_info_t));
    info->trans_seg_list = NULL;
    info->old_value_items = NULL;
    for(int i = 0; i < numsegs; i++) {
        // Check if the segment is in our global list
        seg_t* segment = utility_find_segment(segbases[i]);
        if(segment == NULL) {
            printf("@rvm_begin_trans: Error, segbase not found\n");
            return -1;
        }
        
        // Add segment to transaction info
        info->trans_seg_list = g_list_append(info->trans_seg_list, segment);
        printf("@rvm_begin_trans: Successfully added segment %s to transaction\n", segment->name);
    }

    rvm_t* rvm1 = (rvm_t*)malloc(sizeof(rvm_t));
    char* dir_path = (char*)malloc(GLOBAL_FILENAME_LIMIT * sizeof(char));
    strcpy(dir_path, rvm.dir_path);
    rvm1->dir_path = dir_path;

    char* log_path1 = (char*)malloc(GLOBAL_FILENAME_LIMIT * sizeof(char));
    strcpy(log_path1, rvm.log_path);
    rvm1->log_path = log_path1;
    info->rvm = rvm1;
    int* tid2 = (int*) malloc(sizeof(int));
    *tid2 = tid;
    
    // Insert the mapping (tid -> info) into our hash table
    g_hash_table_insert(GLOBAL_transaction_hashtable, tid2, info);
    printf("@rvm_begin_trans: Assigned id %d at start of transaction\n", tid);
    return tid;
}

void rvm_about_to_modify(trans_t tid, void* segbase, int offset, int size) {
    // Again, check if the segment is in our global list
    seg_t* segment = utility_find_segment(segbase);
    if(segment == NULL) {
        printf("@rvm_about_to_modify: Error, segbase not found\n");
        abort();
    }
    
    // Create trans_item_t object, allocate memory, and populate
    trans_item_t* item = (trans_item_t*)malloc(sizeof(trans_item_t));
    item->seg = segment;
    item->offset = offset;
    item->size = size;
    item->old_value = malloc(size);
    memcpy(item->old_value, segbase + offset, size);

    // Lookup information in our global hashtable and save old values
    trans_info_t* info = g_hash_table_lookup(GLOBAL_transaction_hashtable, &tid);
    info->old_value_items = g_list_append(info->old_value_items, item);
        printf("@rvm_about_to_modify: Saved old values for segment %s\n", segment->name);
}

void rvm_commit_trans(trans_t tid) {
    // As in the previous function, first lookup information in our global hashtable
    trans_info_t* info = g_hash_table_lookup(GLOBAL_transaction_hashtable, &tid);
    GList* new_segment_list = info->trans_seg_list;
    
    // Variables
    seg_t* segment;
    FILE* file_log;
    
    // Open log file
    char directory_path[GLOBAL_FILENAME_LIMIT];
    snprintf(directory_path, GLOBAL_FILENAME_LIMIT, "%s/%s", info->rvm->dir_path, GLOBAL_LOG_FILE_NAME);
    file_log = fopen(directory_path, "a");
    
    // Write to log file
    for(GList* iterator = new_segment_list; iterator; iterator = iterator -> next) {
        segment = (seg_t*)iterator->data;
        // Segment name and size with ~ as a delimiter, with newline at the end
        fprintf(file_log, "%s~%d~", segment->name, segment->size);
        fwrite(segment->mem, segment->size, 1, file_log);
        fprintf(file_log, "\n");
    }

    fclose(file_log);
    printf("@rvm_commit_trans: Successfully committed transaction\n");

}
void rvm_abort_trans(trans_t tid) {
    // Yet again, we start by looking up the id in our global hashtable
    trans_info_t* info = g_hash_table_lookup(GLOBAL_transaction_hashtable, &tid);
    // Traverse backwards and write
    for(GList* iterator = g_list_last(info->old_value_items); iterator != NULL; iterator = g_list_previous(iterator)) {
        trans_item_t* item = (trans_item_t*)iterator->data;
        seg_t* segment = item->seg;
        fwrite(segment->mem, segment->size, 1, stderr /* TODO: insert approp message to be added to stderr */);
        fwrite(item->old_value, item->size, 1, stderr /* TODO: same as above */);
        memcpy((segment->mem) + item->offset, item->old_value, item->size);

    }
    
    printf("@rvm_abort_trans: Successfully aborted transaction\n");
}

void rvm_truncate_log(rvm_t rvm) {
    const char empty_char[2] = "";
    utility_dump_to_disk(rvm, empty_char, 1);
    
    printf("@rvm_truncate_log: Successfully truncated log\n");
}

void rvm_destroy(rvm_t rvm, const char *segname) {
    // Create command to execute
    char command[GLOBAL_FILENAME_LIMIT + 10];
    snprintf(command, 50, "rm ./%s/%s.seg", rvm.dir_path, segname);
    
    // Execute command through system call
    system(command);
    
    printf("@rvm_destroy: Deleted store for segment %s\n", segname);
}


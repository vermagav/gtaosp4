#ifndef __LIBRVM_INTERNAL__
#define __LIBRVM_INTERNAL__

// @Citation: We use the GList library for STL like data structures
// in the C programming language. All of the usage was borrowed from
// this reference page: https://developer.gnome.org/glib/2.37/glib-Doubly-Linked-Lists.html
#include <glib.h>

// Constants used throughout the RVM project
#define GLOBAL_LOG_FILE_NAME "transaction.log"
#define GLOBAL_FILENAME_LIMIT 128
#define GLOBAL_MESSAGE_LENGTH 1024
#define GLOBAL_MAX_SEGMENT_SIZE 100000

// Data structure to hold the actual segment
typedef struct seg {
    const char* name;
    int size;
    void* mem;
} seg_t;

// This has to be convertible to int as per project specification
typedef int trans_t;

// Data structure to hold directory and log paths 
typedef struct rvm {
    const char* dir_path;
    const char* log_path;
} rvm_t;

// Each individual transaction (TODO refine comment)
typedef struct trans_item {
    seg_t* seg;
    int size;
    int offset;
    void* old_value;
} trans_item_t;

// Information stored about a given transaction (TODO refine comment)
typedef struct trans_info {
    GList* trans_seg_list;
    GList* old_value_items;
    rvm_t* rvm;
} trans_info_t;

// A GList list holding all segments
extern GList* GLOBAL_segment_list;

// A GHashTable that maps transaction ID to transaction information
extern GHashTable* GLOBAL_transaction_hashtable;

#endif // __LIBRVM_INTERNAL__

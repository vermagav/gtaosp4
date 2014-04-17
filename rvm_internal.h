#ifndef __LIBRVM_INTERNAL__
#define __LIBRVM_INTERNAL__


#include <list>
#define LOG_NAME "txn.log"
#define FILE_NAME_LEN 50
#define MSG_LEN 1000


typedef struct rvm{
    const char* dir_path;
    const char* log_path;
}rvm_t;

typedef struct seg{
    const char* name;
    int size;
    void* mem;
}seg_t;

typedef struct trans_item{
    seg_t* seg;
    int size;
    int offset;
    void* old_value;
}trans_item_t;

typedef struct trans_info{
    std::list* trans_seg_list;
    std::list* old_value_items;
    rvm_t* rvm;
}trans_info_t;

typedef int trans_t;

#endif
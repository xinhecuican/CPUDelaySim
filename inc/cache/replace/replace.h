#ifndef REPLACE_H
#define REPLACE_H
#include "common/base.h"
#include <stdarg.h>

class Replace : public Base {
public:
    virtual ~Replace() {}
    /**
     * @brief cache hit
     * 
     * @param set The set to insert.
     * @param id The id to insert.
     */
    virtual void insert(int set, int id) = 0;
    /**
     * @brief remove cacheline
     * 
     * @param set The set to clear.
     * @param id The id to clear.
     */
    virtual void clear(int set, int id) = 0;
    /**
     * @brief cache lookup
     * 
     * @param set The set to get.
     * @return int The id that is least recently used.
     */
    virtual int get(int set) = 0;
    virtual void setParams(int arg_num, ...) = 0;
};

#endif

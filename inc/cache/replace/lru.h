#ifndef LRU_H
#define LRU_H
#include "replace.h"

class LRUReplace : public Replace {
public:
    void insert(int set, int id) override;
    void clear(int set, int id) override;
    int get(int set) override;
    void setParams(int arg_num, ...) override;
private:
    /**
     * @ingroup config
     * @brief set size
     */
    int set;
    /**
     * @ingroup config
     * @brief way size
     */ 
    int way;
    std::vector<std::list<int>> lru_list;
    std::vector<std::map<int, std::list<int>::iterator>> lru_map;
};

REGISTER_CLASS(LRUReplace)


#endif
#ifndef PRED_HISTORY_HISTORY_H
#define PRED_HISTORY_HISTORY_H
#include "common/base.h"
#include <unordered_map>

class History : public Base {
public:
    virtual ~History() = default;
    virtual int getMetaSize() = 0;
    virtual void update(bool speculative, bool real_taken, uint64_t real_pc, InstType type, void* meta) = 0;
    virtual void getMeta(void* meta) = 0;
};

class HistoryManager : public Base {
public:
    void load() override;
    void afterLoad() override;
    void update(bool speculative, bool real_taken, uint64_t real_pc, InstType type, void* meta);
    int getMetaSize();
    void getMeta(void* meta);

    template <typename T>
    T* getHistory() {
        for (auto history : histories) {
            T* t = dynamic_cast<T*>(history);
            if (t) {
                return t;
            }
        }
        return nullptr;
    }

private:
    std::vector<History*> histories;

    int meta_size = 0;
};

#endif
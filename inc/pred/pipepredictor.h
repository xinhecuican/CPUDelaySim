#ifndef PRED_SIMPLEPREDICTOR_H
#define PRED_SIMPLEPREDICTOR_H
#include "pred/predictor.h"

class PipePredictor : public Predictor {
public:
    void load() override;
    int predict(uint64_t pc, DecodeInfo* info, uint64_t& next_pc, uint8_t& size, bool& taken, bool stall) override;
};

#endif
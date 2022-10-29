#include "grbl.h"

void ModSyringeContent(uint8_t transfer_type);

void SyringeCycle(uint8_t cycle_type);

void SyringePurge();

void Mix(uint8_t cycles);

//void NeedleWash(uint8_t seconds, uint8_t station);

void Aspirate();

uint8_t Inject(uint8_t transfer_type);

uint8_t PickUpTip(uint16_t tip_no);

uint8_t RemoveTip();

void NeedlePark();

void NeedleUnpark();

uint8_t FCGoNext();

void FCGoCollectPos();

void FCGoVial(uint16_t vial);

//check for max needle penetration/Z
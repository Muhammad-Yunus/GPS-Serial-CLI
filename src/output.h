#ifndef OUTPUT_H
#define OUTPUT_H

#include "gps.h"
#include <stdio.h>

void output_tabular(const gps_t* gps, FILE* fp);
void output_json(const gps_t* gps, FILE* fp);

#endif

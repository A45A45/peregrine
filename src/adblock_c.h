#ifndef ADBLOCK_C_H
#define ADBLOCK_C_H

#include <stdbool.h>

int adblock_init_engine(const char **filter_lists, int count);
int adblock_should_block(const char *url, const char *source_url, const char *request_type);

#endif // ADBLOCK_C_H

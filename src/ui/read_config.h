//
// Created by lakinduakash on 14/04/19.
//

#ifndef WIHOTSPOT_READ_CONFIG_H
#define WIHOTSPOT_READ_CONFIG_H

#include "h_prop.h"

#ifdef __cplusplus
extern "C" {
#endif

int read_config_file();
static void setConfigValues(const char * key, char *value);
ConfigValues* getConfigValues(void);
#ifdef __cplusplus
}
#endif



#endif //WIHOTSPOT_READ_CONFIG_H

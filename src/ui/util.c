//
// Created by lakinduakash on 16/04/19.
//

#include <string.h>
#include "util.h"

int find_str(char *find, const char **array, int length) {
    int i;

    for ( i = 0; i < length; i++ ) {
        if (strcmp(array[i], find) == 0) {
            return i;
        }
    }


    return -1;

}

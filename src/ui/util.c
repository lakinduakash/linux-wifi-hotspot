//
// Created by lakinduakash on 16/04/19.
//
/*
Copyright (c) 2019, lakinduaksh
        All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
        FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
        CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include "util.h"
#include <regex.h>

regex_t regex;
int reti;
char msgbuf[100];

int find_str(char *find, const char **array, int length) {
    int i;

    for ( i = 0; i < length; i++ ) {
        if (strcmp(array[i], find) == 0) {
            return i;
        }
    }


    return -1;

}


void rand_str(char *dest, size_t length) {
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        size_t index = (size_t) ((double) rand() / RAND_MAX * (sizeof charset - 1));
        *dest++ = charset[index];
    }
    *dest = '\0';
}


int isValidMacAddress(const char* mac) {
    int i = 0;
    int s = 0;

    while (*mac) {
        if (isxdigit(*mac)) {
            i++;
        }
        else if (*mac == ':' || *mac == '-') {

            if (i == 0 || i / 2 - 1 != s)
                break;

            ++s;
        }
        else {
            s = -1;
        }


        ++mac;
    }

    return (i == 12 && (s == 5 || s == 0));
}


int isValidAcceptedMacs(const char *macs){

    /* Compile regular expression */
reti = regcomp(&regex, "^(((([0-9A-Fa-f]{2}):){5}[0-9A-Fa-f]{2}\\s*)(^((([0-9A-Fa-f]{2}):){5}[0-9A-Fa-f]{2}\\s*))*)$", REG_EXTENDED);
if (reti) {
    //printf( "Could not compile regex\n");
    return -1;
}

/* Execute regular expression */
reti = regexec(&regex, macs, 0, NULL, 0);
if (!reti) {
    return 0;
}
else if (reti == REG_NOMATCH) {
    //puts("Invalid mac addresses");
    return -1;
}
else {
    regerror(reti, &regex, msgbuf, sizeof(msgbuf));
    //printf("Regex match failed: %s\n", msgbuf);
    return -1;
}

}

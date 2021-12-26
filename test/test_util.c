#include <assert.h>
#include <stdio.h>
#include <util.h>

int main(int argc, char *argv[]){

    assert(0 == isValidIPaddress("192.12.23.123"));
    assert(0 == isValidIPaddress("255.255.255.255"));
    assert(-1 == isValidIPaddress("255.255.255.256"));
    assert(-1 == isValidIPaddress("255.255.255.2551"));
    assert(-1 == isValidIPaddress("192.168.12"));

    return 0;
}
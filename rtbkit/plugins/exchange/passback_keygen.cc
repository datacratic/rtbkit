/* passback_keygen.cc                                    -*- C++ -*-
   Michael Burkat, 7 Octobre 2013
   Copyright (c) 2014 Datacratic Inc.  All rights reserved.

*/

#include <string>
#include <iostream>
#include "passback_encryption.h"

using namespace std;

/*****************************************************************************/
/* PASSBACK ENCRYPTION RUNNER                                                */
/*****************************************************************************/

int main() {
    RTBKIT::PassbackEncryption pe;
    
    string key = pe.generateKey();
    string iv = pe.generateIV();

    cout << "Key: " << key << endl;
    cout << "IV:  " << iv << endl;

    return 0;
}


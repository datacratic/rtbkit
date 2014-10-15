/* passback_encryption.h                                    -*- C++ -*-
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

    string account = "test1:subtest1";
        
    string encrypted = pe.encrypt(account, key, iv);
    cout << "encrypted: " << encrypted << endl;
    
    string decrypted = pe.decrypt(encrypted, key, iv);
    cout << "decrypted: " << decrypted << endl;

    return 0;
}


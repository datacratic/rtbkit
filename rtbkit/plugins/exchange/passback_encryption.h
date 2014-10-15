/* passback_encryption.h                                    -*- C++ -*-
   Michael Burkat, 7 Octobre 2013
   Copyright (c) 2014 Datacratic Inc.  All rights reserved.

*/

#pragma once

#include <string>
#include <iostream>

namespace RTBKIT {

/*****************************************************************************/
/* PASSBACK ENCRYPTION                                                       */
/*****************************************************************************/

/**
 * Used to encrypt and decrypt passbacks in exchange connector
*/

struct PassbackEncryption {

    typedef unsigned char byte;
    
    std::string encrypt(const std::string & passback,
                        const std::string & key,
                        const std::string & iv);

    std::string decrypt(const std::string & passback,
                        const std::string & key,
                        const std::string & iv);

    std::string byteToStr(byte * encode);

    std::string hexEncode(const std::string & decoded);
    std::string hexDecode(const std::string & encoded);
    
    std::string generateKey();

    std::string generateIV();

};

} // namespace RTBKIT

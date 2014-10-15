/* passback_encryption.cc                                   -*- C++ -*-
   Michael Burkat, 7 Octobre 2013
   Copyright (c) 2014 Datacratic Inc.  All rights reserved.

*/

#include "passback_encryption.h"
#include "cryptopp/modes.h"
#include "cryptopp/crc.h"
#include "cryptopp/osrng.h"
#include "cryptopp/hex.h"
#include "cryptopp/aes.h"

using namespace std; 
using namespace CryptoPP;

namespace RTBKIT {

/*****************************************************************************/
/* PASSBACK ENCRYPTION                                                       */
/*****************************************************************************/

/**
 * Used to encrypt and decrypt passbacks in exchange connector
*/

string
PassbackEncryption::encrypt(const string & passback, const string & key, const string & iv) {
    const byte * bk = reinterpret_cast<const byte *>(key.c_str());
    const byte * biv =  reinterpret_cast<const byte *>(iv.c_str());

    CFB_Mode<AES>::Encryption e(bk, AES::DEFAULT_KEYLENGTH, biv);

    string cipher;
    StringSource(passback, true, 
        new StreamTransformationFilter(e,
            new StringSink(cipher)
        )
    );
    return hexEncode(cipher);
}

string
PassbackEncryption::decrypt(const string & passback, const string & key, const string & iv) {
    const byte * bk = reinterpret_cast<const byte *>(key.c_str());
    const byte * biv =  reinterpret_cast<const byte *>(iv.c_str());

    CFB_Mode<AES>::Decryption d(bk, AES::DEFAULT_KEYLENGTH, biv);
    
    string recovered;
    StringSource s(hexDecode(passback), true, 
        new StreamTransformationFilter(d,
            new StringSink(recovered)
        )
    );
    return recovered;
}

string
PassbackEncryption::hexEncode(const string & decoded) {
    string encoded;
    StringSource(decoded, true,
        new HexEncoder(
            new StringSink(encoded)
        )
    );
    return encoded;
}
    
string
PassbackEncryption::hexDecode(const string & encoded) {
    string decoded;
    StringSource(encoded, true,
        new HexDecoder(
            new StringSink(decoded)
        )
    );
    return decoded;
}

string
PassbackEncryption::byteToStr(byte * encode) {
    string encoded;
    encoded.clear();
    StringSource(encode, AES::DEFAULT_KEYLENGTH, true,
            new HexEncoder(
                new StringSink(encoded)
            )
    );
    return encoded;
}

string
PassbackEncryption::generateKey() {
    AutoSeededRandomPool rnd;
    byte key[AES::DEFAULT_KEYLENGTH];
    rnd.GenerateBlock(key, sizeof(key));

    return byteToStr(key);
}

string
PassbackEncryption::generateIV() {
    AutoSeededRandomPool rnd;
    byte iv[AES::BLOCKSIZE];
    rnd.GenerateBlock(iv, sizeof(iv));
    
    return byteToStr(iv); 
}

} // namespace RTBKIT

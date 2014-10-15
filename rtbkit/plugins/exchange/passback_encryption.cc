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
#include <iostream>

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
    return addDigest(cipher);
}

string
PassbackEncryption::decrypt(const string & passback, const string & key, const string & iv) {
    const byte * bk = reinterpret_cast<const byte *>(key.c_str());
    const byte * biv =  reinterpret_cast<const byte *>(iv.c_str());

    CFB_Mode<AES>::Decryption d(bk, AES::DEFAULT_KEYLENGTH, biv);
    
    string noDigest = removeDigest(hexDecode(passback));
    if (noDigest == "")
        return "";

    string recovered;
    StringSource s(noDigest, true, 
        new StreamTransformationFilter(d,
            new StringSink(recovered)
        )
    );
    return recovered;
}

string PassbackEncryption::addDigest(const string & encrypted) {
    return hexEncode(encrypted) + digest(encrypted);
}

string PassbackEncryption::removeDigest(const string & digested) {
    if (digested.size() <= CRC32::DIGESTSIZE) {
        return "";
    }

    const string digestStr = digested.substr(digested.size() - CRC32::DIGESTSIZE); 
    const string encrypted = digested.substr(0, digested.size() - CRC32::DIGESTSIZE);

    if (hexEncode(digestStr) == digest(encrypted))
        return encrypted;
    else
        return "";
}

string PassbackEncryption::digest(const string & encrypted) {
    byte digest[CRC32::DIGESTSIZE];
    const byte * bencrypt = reinterpret_cast<const byte *>(encrypted.c_str());
    CRC32 crc;
    crc.CalculateDigest(digest, bencrypt, encrypted.size());
    return byteToStr(digest, CRC32::DIGESTSIZE);
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
PassbackEncryption::byteToStr(byte * encode, size_t size) {
    string encoded;
    encoded.clear();
    StringSource(encode, size, true,
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

    return byteToStr(key, AES::DEFAULT_KEYLENGTH);
}

string
PassbackEncryption::generateIV() {
    AutoSeededRandomPool rnd;
    byte iv[AES::BLOCKSIZE];
    rnd.GenerateBlock(iv, sizeof(iv));
    
    return byteToStr(iv, AES::BLOCKSIZE);
}

} // namespace RTBKIT

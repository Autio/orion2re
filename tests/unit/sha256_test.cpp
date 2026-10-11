#include "game/sha256.h"

#include <doctest/doctest.h>

#include <string>

TEST_CASE("SHA-256 matches standard vectors")
{
    CHECK(orion2re::crypto::sha256_hex("") ==
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(orion2re::crypto::sha256_hex("abc") ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(orion2re::crypto::sha256_hex(
              "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST_CASE("SHA-256 supports binary input and reusable binary digests")
{
    const std::string binary("a\0b", 3);
    const auto digest = orion2re::crypto::sha256(binary);
    CHECK(orion2re::crypto::sha256_hex(digest) ==
          "59b271ae1bbcb1d31d41929817f4b16fb439eb4f31520b5ad1d5ce98920a7138");
}
// 对于 decompress_zlib
#include <stdexcept>
#include <string>
#include <zlib.h>
// 对于 compute_sha1
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <string>
// 对于 compress_zlib
#include <stdexcept>
#include <string>
#include <zlib.h>
std::string decompress_zlib(const std::string &compressed);
std::string compute_sha1(const std::string &data);
std::string compress_zlib(const std::string &data);
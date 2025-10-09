#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <zlib.h>
#include <vector>
#include <openssl/sha.h>
#include <vector>
#include <algorithm>
#include <string.h>
#include <curl/curl.h>

#define CHUNK 16384 //16KB
using namespace std;
void compressFile(const std::string data, uLong *bound, unsigned char *dest) 
{
    compress(dest, bound, (const Bytef *)data.c_str(), data.size());
}
std::string sha_file(std::string data) 
{
    unsigned char hash[20];
    SHA1((unsigned char *)data.c_str(), data.size(), hash);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    //std::cout << ss.str() << std::endl;
    return ss.str();
}
string hash_object(std::string file) 
{
    // Read file contents
    std::ifstream t(file);
    std::stringstream data;
    data << t.rdbuf();
    // Create blob content string
    std::string content = "blob " + std::to_string(data.str().length()) + '\0' + data.str();
    // Calculate SHA1 hash
    std::string buffer = sha_file(content);
    // Compress blob content
    uLong bound = compressBound(content.size());
    unsigned char compressedData[bound];
    compressFile(content, &bound, compressedData);
    // Write compressed data to .git/objects
    std::string dir = ".git/objects/" + buffer.substr(0,2);
    std::filesystem::create_directory(dir);
    std::string objectPath = dir + "/" + buffer.substr(2);
    std::ofstream objectFile(objectPath, std::ios::binary);
    objectFile.write((char *)compressedData, bound);
    objectFile.close();
    return buffer;
}
std::string write_tree(const std::string dir_path)
{
    namespace fs = std::filesystem;
    std::vector<std::pair<std::string, std::string>> entries;
    std::string mode;
    std::string sha1;
    for (const auto &entry : fs::directory_iterator(dir_path))
    {
        std::string name = entry.path().filename().string();
        if (name == ".git")
            continue;
        if (entry.is_directory())
        {
            mode = "40000";
            sha1 = write_tree(entry.path().string());
        }
        else if (entry.is_regular_file())
        {
            mode = "100644";
            sha1 = hash_object(entry.path().string());
        }
        if (!sha1.empty())
        {
            std::string binary_sha;
            for (size_t i = 0; i < sha1.length(); i += 2)
            {
                std::string byte_string = sha1.substr(i, 2);
                char byte = static_cast<char>(std::stoi(byte_string, nullptr, 16));
                binary_sha.push_back(byte);
            }
            std::string entry_data = mode + " " + name + '\0' + binary_sha;
            entries.push_back({name, entry_data});
        }
    }
    sort(entries.begin(), entries.end());
    std::string tree_content;
    for (auto &it : entries)
    {
        tree_content += it.second;
    }
    std::string tree_store = "tree " + std::to_string(tree_content.size()) + '\0' + tree_content;
    unsigned char hash[20];
    SHA1((unsigned char *)tree_store.c_str(), tree_store.size(), hash);
    std::string tree_sha;
    for (int i = 0; i < 20; i++)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(hash[i]);
        tree_sha += ss.str();
    }
    std::string tree_dir = ".git/objects/" + tree_sha.substr(0, 2);
    std::filesystem::create_directory(tree_dir);
    std::string tree_filepath = tree_dir + "/" + tree_sha.substr(2);
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK)
    {
        throw(std::runtime_error("deflateInit failed while compressing."));
    }
    zs.next_in = (Bytef *)tree_store.c_str();
    zs.avail_in = tree_store.size();
    int ret;
    char outBuffer[32768];
    std::string outstring;
    do
    {
        zs.next_out = reinterpret_cast<Bytef *>(outBuffer);
        zs.avail_out = sizeof(outBuffer);
        ret = deflate(&zs, Z_FINISH);
        if (outstring.size() < zs.total_out)
        {
            // TODO: check other approaches
            outstring.insert(outstring.end(), outBuffer, outBuffer + zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);
    deflateEnd(&zs);
    if (ret != Z_STREAM_END)
    {
        throw(std::runtime_error("Exception during zlib compression: " + std::to_string(ret)));
    }
    std::ofstream outfile(tree_filepath, std::ios::binary);
    outfile.write(outstring.c_str(), outstring.size());
    outfile.close();
    return tree_sha;
}
std::pair<std::string, std::string> object_path_from_sha(const std::string& sha) {
    std::string folder_name = sha.substr(0, 2);
    std::string file_name = sha.substr(2);
    std::string folderpath = ".git/objects/" + folder_name;
    std::string filepath = ".git/objects/" + folder_name + "/" + file_name;
    return std::make_pair(folderpath, filepath);
}
void commit_tree(string treeSha, string parSha, string comMsg)
{
    std::ostringstream commit_body;
    commit_body << "tree " << treeSha << '\n';
    commit_body << "parent " << parSha << '\n';
    commit_body << "author XXX YYY <xxx.yyy@gmail.com> 1620000000 +0000\n";
    commit_body << "committer XXX YYY <xxx.yyy@gmail.com> 1620000000 +0000\n";
    commit_body << '\n' << comMsg << '\n';
    std::string commit = "commit " + std::to_string(commit_body.str().size()) + '\x00' + commit_body.str();
    std::string tree_sha = sha_file(commit);
    std::cout << tree_sha;
    auto [folderpath, filepath] = object_path_from_sha(tree_sha);
    std::filesystem::create_directories(folderpath);
    uLong bound = compressBound(commit.size());
    unsigned char compressedData[bound];
    compressFile(commit, &bound, compressedData);
    // std::string compressed_commit = compress_blob(commit);
    std::ofstream output_file(filepath, std::ios::binary);
    output_file.write((char *)compressedData, bound);
    output_file.close();
}
bool git_init (const std::string& dir) {
    std::cout << "git init \n";
    try {
        std::filesystem::create_directory(dir + "/.git");
        std::filesystem::create_directory(dir + "/.git/objects");
        std::filesystem::create_directory(dir + "/.git/refs");
        std::ofstream headFile(dir + "/.git/HEAD");
        if (headFile.is_open()) { // create .git/HEAD file
            headFile << "ref: refs/heads/master\n"; // write to the headFile
            headFile.close();
        } else {
            std::cerr << "Failed to create .git/HEAD file.\n";
            return false;
        }
       
        std::cout << "Initialized git directory in " << dir << "\n";
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return false;
    }
}
std::string compute_sha1(const std::string& data, bool print_out = false) {
    unsigned char hash[20]; // 160 bits long for SHA1
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    if (print_out)  {
        std::cout << ss.str() << std::endl;
    }
    return ss.str();
}
int compress(FILE* input, FILE* output) {
    // Initialize compression stream.
    //std::cout << "Initializing compression stream.\n";
    z_stream stream = {0};
    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        std::cerr << "Failed to initialize compression stream.\n";
        return EXIT_FAILURE;
    }
    char in[CHUNK];
    char out[CHUNK];
    int ret;
    int flush;
    do {
        stream.avail_in = fread(in, 1, CHUNK, input);
        stream.next_in = reinterpret_cast<unsigned char*>(in);
        if (ferror(input)) {
            (void)deflateEnd(&stream);  // Free memory
            std::cerr << "Failed to read from input file.\n";
            return EXIT_FAILURE;
        }
        flush = feof(input) ? Z_FINISH : Z_NO_FLUSH;
        do {
            stream.avail_out = CHUNK;
            stream.next_out = reinterpret_cast<unsigned char*>(out);
            ret = deflate(&stream, flush);
            if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                (void)deflateEnd(&stream);  // Free memory
                std::cerr << "Failed to compress file.\n";
                return EXIT_FAILURE;
            }
            size_t have = CHUNK - stream.avail_out;
            if (fwrite(out, 1, have, output) != have || ferror(output)) {
                (void)deflateEnd(&stream);  // Free memory
                std::cerr << "Failed to write to output file.\n";
                return EXIT_FAILURE;
            }
        } while (stream.avail_out == 0);
    } while (flush != Z_FINISH);
    // Clean up and check for errors
    if (deflateEnd(&stream) != Z_OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
void compress_and_store(const std::string& hash, const std::string& content, std::string dir = ".") {
    FILE* input = fmemopen((void*) content.c_str(), content.length(), "rb");
    std::string hash_folder = hash.substr(0, 2);
    std::string object_path = dir + "/.git/objects/" + hash_folder + '/';
    if (!std::filesystem::exists(object_path)) {
        std::filesystem::create_directories(object_path);
    }
    std::string object_file_path = object_path + hash.substr(2);
    if (!std::filesystem::exists(object_file_path)) {
        FILE* output = fopen(object_file_path.c_str(), "wb");
        if (compress(input, output) != EXIT_SUCCESS) {
            std::cerr << "Failed to compress data.\n";
            return;
        }
        fclose(output);
    }
    fclose(input);
}
int read_length(const std::string& pack, int* pos) {
    int length = 0;
    length |= pack[*pos] & 0x0F;
    if (pack[*pos] & 0x80) {
        (*pos)++;
        while (pack[*pos] & 0x80) {
            length <<= 7;
            length |= pack[*pos] & 0x7F;
            (*pos)++;
        }
        length <<= 7;
        length |= pack[*pos];
    }
    (*pos)++;
    return length;
}
std::string apply_delta(const std::string& delta_contents, const std::string& base_contents) {
    std::string reconstructed_object;
    int current_position_in_delta = 0;
    // read and skip the length of the base object
    read_length(delta_contents, &current_position_in_delta);
    read_length(delta_contents, &current_position_in_delta);
    while (current_position_in_delta < delta_contents.length()) {
        unsigned char current_instruction = delta_contents[current_position_in_delta++];
        if (current_instruction & 0x80) {
            int copy_offset = 0;
            int copy_size = 0;
            int bytes_processed_for_offset = 0;
            for (int i = 3; i >= 0; i--) {
                copy_offset <<= 8;
                if (current_instruction & (1 << i)) {
                    copy_offset += static_cast<unsigned char>(delta_contents[current_position_in_delta + i]);
                    bytes_processed_for_offset++;
                }
            }
            int bytes_processed_for_size = 0;
            // calculate copy size from the delta contents
            for (int i = 6; i >= 4; i--) {
                copy_size <<= 8;
                if (current_instruction & (1 << i)) {
                    copy_size += static_cast<unsigned char>(delta_contents[current_position_in_delta + i - (4 - bytes_processed_for_offset)]);
                    bytes_processed_for_size++;
                }
            }
            // default size to 0x100000 if no size was specified
            if (copy_size == 0) {
                copy_size = 0x100000;
            }
            reconstructed_object += base_contents.substr(copy_offset, copy_size);
            current_position_in_delta += bytes_processed_for_size + bytes_processed_for_offset;
        } else {
            // direct add insturction, the highest bit is not set
            int add_size = current_instruction & 0x7F;
            reconstructed_object += delta_contents.substr(current_position_in_delta, add_size);
            current_position_in_delta += add_size;
        }
    }
    return reconstructed_object;
}
std::string digest_to_hash(const std::string& digest) {
    std::stringstream ss;
    for (unsigned char c : digest) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return ss.str();
}
std::string decompress_string(const std::string& compressed_str) {
    z_stream d_stream;
    memset(&d_stream, 0, sizeof(d_stream));
    if (inflateInit(&d_stream) != Z_OK) {
        throw(std::runtime_error("inflateInit failed while decompressing."));
    }
    d_stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed_str.data()));
    d_stream.avail_in = compressed_str.size();
    int status;
    const size_t buffer_size = 32768; // 32KB
    char buffer[buffer_size];
    std::string decompressed_str;
    do {
        d_stream.next_out = reinterpret_cast<Bytef*>(buffer);
        d_stream.avail_out = buffer_size;
        status = inflate(&d_stream, 0);
        if (decompressed_str.size() < d_stream.total_out) {
            decompressed_str.append(buffer, d_stream.total_out - decompressed_str.size());
        }
    } while (status == Z_OK);
    if (inflateEnd(&d_stream) != Z_OK) {
        throw(std::runtime_error("inflateEnd failed while decompressing."));
    }
    if (status != Z_STREAM_END) {
        std::ostringstream oss;
        oss << "Exception during zlib decompression: (" << status << ") " << d_stream.msg;
        throw(std::runtime_error(oss.str()));
    }
    return decompressed_str;
}
std::string compress_string(const std::string& input_str) {
    z_stream c_stream;
    memset(&c_stream, 0, sizeof(c_stream));
    if (deflateInit(&c_stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        throw(std::runtime_error("deflateInit failed while compressing."));
    }
    c_stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input_str.data()));
    c_stream.avail_in = input_str.size();
    int status;
    const size_t buffer_size = 32768; // 32KB
    char buffer[buffer_size];
    std::string compressed_str;
    do {
        c_stream.next_out = reinterpret_cast<Bytef*>(buffer);
        c_stream.avail_out = sizeof(buffer);
        status = deflate(&c_stream, Z_FINISH);
        if (compressed_str.size() < c_stream.total_out) {
            compressed_str.append(buffer, c_stream.total_out - compressed_str.size());
        }
    } while (status == Z_OK);
    if (deflateEnd(&c_stream) != Z_OK) {
        throw(std::runtime_error("deflateEnd failed while compressing."));
    }
    if (status != Z_STREAM_END) {
        std::ostringstream oss;
        oss << "Exception during zlib compression: (" << status << ") " << c_stream.msg;
        throw(std::runtime_error(oss.str()));
    }
    return compressed_str;
}
size_t write_callback(void* received_data, size_t element_size, size_t num_element, void* userdata) {
    size_t total_size = element_size * num_element;
    std::string received_text((char*) received_data, num_element);
    std::string* master_hash = (std::string*) userdata;
    if (received_text.find("servie=git-upload-pack") == std::string::npos) {
        size_t hash_pos = received_text.find("refs/heads/master\n");
        if (hash_pos != std::string::npos) {
            *master_hash = received_text.substr(hash_pos - 41, 40);
        }
    }
    return total_size;
}
size_t pack_data_callback(void* received_data, size_t element_size, size_t num_element, void* userdata) {
    std::string* accumulated_data = (std::string*) userdata;
    *accumulated_data += std::string((char*) received_data, num_element);
    return element_size * num_element;
}
std::pair<std::string, std::string> curl_request (const std::string& url) {
    CURL* handle = curl_easy_init();
    if (handle) {
        // fetch info/refs
        curl_easy_setopt(handle, CURLOPT_URL, (url + "/info/refs?service=git-upload-pack").c_str());
        
        std::string packhash;
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*) &packhash);
        curl_easy_perform(handle);
        curl_easy_reset(handle);
        // fetch git-upload-pack
        curl_easy_setopt(handle, CURLOPT_URL, (url + "/git-upload-pack").c_str());
        std::string postdata = "0032want " + packhash + "\n" +
                               "00000009done\n";
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postdata.c_str());
        std::string pack;
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*) &pack);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, pack_data_callback);
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-git-upload-pack-request");
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
        curl_easy_perform(handle);
        // clean up
        curl_easy_cleanup(handle);
        curl_slist_free_all(headers);
        return {pack, packhash};
    }
    else {
        std::cerr << "Failed to initialize curl.\n";
        return {};
    }
}
int decompress(FILE* input, FILE* output) {
    // initialize decompression stream
    //std::cout << "Initializing decompression stream.\n";
    z_stream stream = {0};
    if (inflateInit(&stream) != Z_OK) {
        std::cerr << "Failed to initialize decompression stream.\n";
        return EXIT_FAILURE;
    }
    // initialize decompression variables
    char in[CHUNK];
    char out[CHUNK];
    bool haveHeader = false;
    char header[64];
    int ret;
    do {
        stream.avail_in = fread(in, 1, CHUNK, input); // read from input file
        stream.next_in = reinterpret_cast<unsigned char*>(in); // set input stream
        if (ferror(input)) {
            std::cerr << "Failed to read from input file.\n";
            return EXIT_FAILURE;
        }
        if (stream.avail_in == 0) {
            break;
        }
        do {
            stream.avail_out = CHUNK; // set output buffer size
            stream.next_out = reinterpret_cast<unsigned char*>(out); // set output stream
            ret = inflate(&stream, Z_NO_FLUSH); // decompress
            if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                std::cerr << "Failed to decompress file.\n";
                return EXIT_FAILURE;
            }
            // write header to output file
            unsigned headerLen = 0, dataLen = 0;
            if (!haveHeader) {
                sscanf(out, "%s %u", header, &dataLen);
                haveHeader = true;
                headerLen = strlen(out) + 1;
            }
            // write decompressed data to output file
            if (dataLen > 0) {
                if(fwrite(out + headerLen, 1, dataLen, output) != dataLen) {
                    std::cerr << "Failed to write to output file.\n";
                    return EXIT_FAILURE;
                }
            }
        } while (stream.avail_out == 0);
        
    } while (ret != Z_STREAM_END);
    return inflateEnd(&stream) == Z_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
int cat_file_for_clone(const char* file_path, const std::string& dir, FILE* dest, bool print_out = false) {
    try {
        std::string blob_sha = file_path;
        std::string blob_path = dir + "/.git/objects/" + blob_sha.insert(2, "/");
        if (print_out) std::cout << "blob path: " << blob_path << std::endl;
        FILE* blob_file = fopen(blob_path.c_str(), "rb");
        if (blob_file == NULL) {
            std::cerr << "Invalid object hash.\n";
            return EXIT_FAILURE;
        }
        decompress(blob_file, dest);
        fclose(blob_file);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
void restore_tree(const std::string& tree_hash, const std::string& dir, const std::string& proj_dir) {
    std::string object_path = proj_dir + "/.git/objects/" + tree_hash.substr(0, 2) + '/' + tree_hash.substr(2);
    std::ifstream master_tree(object_path);
    std::ostringstream buffer;
    buffer << master_tree.rdbuf();
    std::string tree_contents = decompress_string(buffer.str());
    tree_contents = tree_contents.substr(tree_contents.find('\0') + 1);
    // iterate over each entry in the tree object
    int pos = 0;
    while (pos < tree_contents.length()) {
        if (tree_contents.find("40000", pos) == pos) {
            pos += 6; // skip the mode 40000
            // extract the directory path
            std::string path = tree_contents.substr(pos, tree_contents.find('\0', pos) - pos);
            pos += path.length() + 1; // skip the path and the null byte
            std::string next_hash = digest_to_hash(tree_contents.substr(pos, 20));
            // create directories and recursively restore the nested tree
            std::filesystem::create_directory(dir + '/' + path);
            restore_tree(next_hash, dir + '/' + path, proj_dir);
            pos += 20; // skip the hash
        } else {
            pos += 7; // skip the mode 100644
            std::string path = tree_contents.substr(pos, tree_contents.find('\0', pos) - pos);
            pos += path.length() + 1; // skip the path and the null byte
            std::string blob_hash = digest_to_hash(tree_contents.substr(pos, 20));
            FILE* new_file = fopen((dir + '/' + path).c_str(), "wb");
            cat_file_for_clone(blob_hash.c_str(), proj_dir, new_file);
            fclose(new_file);
            pos += 20; // skip the hash
        }
    }
}
int clone (std::string url, std::string dir) {
    // create the repository directory and initialize it
    std::filesystem::create_directory(dir);
    if (git_init(dir) != true) {
        std::cerr << "Failed to initialize git repository.\n";
        return EXIT_FAILURE;
    }
    // fetch the repository
    auto [pack, packhash] = curl_request(url);
    // parse the pack file
    int num_objects = 0;
    for (int i=16; i<20; i++) {
        num_objects = num_objects << 8;
        num_objects = num_objects | (unsigned char) pack[i];
    }
    pack = pack.substr(20, pack.length() - 40); // removing the headers of HTTP
    // proecessing object files in a git pack file
    int object_type;
    int current_position = 0;
    std::string master_commit_contents;
    for (int object_index = 0; object_index < num_objects; object_index++) {
        // extract object type from the first byte
        object_type = (pack[current_position] & 112) >> 4; // 112 is 11100000
        // read the object's length
        int object_length = read_length(pack, &current_position);
        // process based on object type
        if (object_type == 6) { // offset deltas: ignore it
            throw std::invalid_argument("Offset deltas not implemented.\n");
        }
        else if (object_type == 7) { // reference deltas
            // process reference deltas
            std::string digest = pack.substr(current_position, 20);
            std::string hash = digest_to_hash(digest);
            current_position += 20;
            // read the base object's contents
            std::ifstream file(dir + "/.git/objects/" + hash.insert(2, "/"));
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string file_contents = buffer.str();
            std::string base_object_contents = decompress_string(file_contents);
            
            // extract and remove the object type
            std::string object_type_extracted = base_object_contents.substr(0, base_object_contents.find(" "));
            base_object_contents = base_object_contents.substr(base_object_contents.find('\0') + 1);
            // apply delta to base object
            std::string delta_contents = decompress_string(pack.substr(current_position));
            std::string reconstructed_contents = apply_delta(delta_contents, base_object_contents);
            // reconstruct the object with its type and length
            reconstructed_contents = object_type_extracted + ' ' + std::to_string(reconstructed_contents.length()) + '\0' + reconstructed_contents;
            // compute the object hash and store it
            std::string object_hash = sha_file(reconstructed_contents);
            compress_and_store(object_hash.c_str(), reconstructed_contents, dir);
            // advance position past the delta data
            std::string compressed_delta = compress_string(delta_contents);
            current_position += compressed_delta.length();
            // update master commits if hash matches
            if (hash.compare(packhash) == 0) {
                master_commit_contents = reconstructed_contents.substr(reconstructed_contents.find('\0'));
            }
        }
        else { // other object types (1: commit, 2: tree, other: blob)
            // process standard objects
            std::string object_contents = decompress_string(pack.substr(current_position));
            current_position += compress_string(object_contents).length();
            // prepare object header
            std::string object_type_str = (object_type == 1) ? "commit " : (object_type == 2) ? "tree " : "blob ";
            object_contents = object_type_str + std::to_string(object_contents.length()) + '\0' + object_contents;
            // store the object and update master commits if hash matches
            std::string object_hash = compute_sha1(object_contents, false);
            std::string compressed_object = compress_string(object_contents);
            compress_and_store(object_hash.c_str(), object_contents, dir);
            if (object_hash.compare(packhash) == 0) {
                master_commit_contents = object_contents.substr(object_contents.find('\0'));
            }
        }
    }
    // restore the tree
    std::string tree_hash = master_commit_contents.substr(master_commit_contents.find("tree") + 5, 40);
    restore_tree(tree_hash, dir, dir);
    return EXIT_SUCCESS;
}
int main(int argc, char *argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    // You can use print statements as follows for debugging, they'll be visible when running tests.
    //std::cout << "Logs from your program will appear here!\n";
    
    if (argc < 2) {
        std::cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }
    
    std::string command = argv[1];
    
    if (command == "init") 
    {
        try {
            std::filesystem::create_directory(".git");
            std::filesystem::create_directory(".git/objects");
            std::filesystem::create_directory(".git/refs");
    
            std::ofstream headFile(".git/HEAD");
            if (headFile.is_open()) {
                headFile << "ref: refs/heads/main\n";
                headFile.close();
            } else {
                std::cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }
    
            std::cout << "Initialized git directory\n";
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    } 
    else if(command == "cat-file")
    {
        if (argc <= 3)
        {
            cerr << "Invalid arguments, required `-p <blob_sha>`\n";
            return EXIT_FAILURE;
        }
        const string flag = argv[2];
        if (flag != "-p")
        {
            std::cerr << "Invalid flag for cat-file, expected `-p`\n";
            return EXIT_FAILURE;
        }
        const string value = argv[3];
        const string dir_name = value.substr(0, 2);
        const string blob_sha = value.substr(2);
        // not secure - technically. - can have LFI
        std::string path = ".git/objects/" + dir_name + "/" + blob_sha;
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open())
        {
            std::cerr << "Failed to open object file?\n";
            return EXIT_FAILURE;
        }
        in.seekg(0, std::ios::end);
        int fileSize = in.tellg();
        in.seekg(0, std::ios::beg);
        if(fileSize > 0) 
        {
            std::vector<Bytef> buffer(fileSize);
            in.read(reinterpret_cast<char*>(buffer.data()), fileSize);
            uLongf decompressedSize = fileSize * 100;
            std::vector<Bytef> decompressedData(decompressedSize);
            int result = uncompress(decompressedData.data(), &decompressedSize, buffer.data(), fileSize);
            if (result == Z_OK) 
            {
                bool isSignFound = false;
                for(int i = 0; i < decompressedSize; i++) 
                {
                    if(isSignFound)  std::cout << decompressedData[i];
                    if(decompressedData[i] == '\0') 
                    {
                        isSignFound = true;
                    }
                }
            }
        }
    }
    else if (command=="hash-object")
    {
        if(argc<3){
            std::cerr << "Less than 3 argument "<< '\n';
            return EXIT_FAILURE;
        }
        std::string hash=argv[3];
        cout<<hash_object(hash)<<endl;
    }
    //The ls-tree command
    else if(command == "ls-tree"){
        std::string flag = "";
        std::string tree_sha = "";
        //if no argument is given
        if(argc<=2){
            std::cerr<<"Invalid Arguments"<< "\n";
            return EXIT_FAILURE;
        }
        //if the flag is not included
        if(argc==3){
            tree_sha = argv[2];
        }else{
            tree_sha = argv[3];
            flag = argv[2];
        }
        //if the flag is wrong
        if(flag.size()!=0 && flag != "--name-only"){
            std::cerr<<"Incorrect flag :"<<flag<<"\nexpected --name-only"<<"\n";
            return EXIT_FAILURE;
        }
        std::string dir_name = tree_sha.substr(0,2);
        std::string file_name = tree_sha.substr(2);
        std::string path = "./.git/objects/" + dir_name + "/" + file_name;
        auto file = std::ifstream(path);
        if(!file.is_open()){
            std::cerr<<"Failed to open file: "<< path << "\n";
            return EXIT_FAILURE;
        }
        auto tree_data = std::string(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
        std::string buf;
        buf.resize(tree_data.size());
        while(true){
            auto buf_size = buf.size();
            auto res =  uncompress(reinterpret_cast<Bytef*>(buf.data()),&buf_size,reinterpret_cast<const Bytef*>(tree_data.data()),tree_data.size());
            if(res == Z_BUF_ERROR){
                buf.resize(buf.size() * 2);
            }
            else if(res != Z_OK){
                std::cerr<<"Failed to decompress file, code:"<<res<<"\n";
                return EXIT_FAILURE;
            }
            else{
                buf.resize(buf_size);
                break;
            }
        }     
        std::string trimmed_data = buf.substr(buf.find('\0')+1);
        std::string line;
        std::vector<std::string> names;
        do{
        
        line = trimmed_data.substr(0,trimmed_data.find('\0'));
        if(line.substr(0,5) == "40000" )
            names.push_back(line.substr(6));
        else
            names.push_back(line.substr(7));
        trimmed_data = trimmed_data.substr(trimmed_data.find('\0')+21);
        }while(trimmed_data.size()>1);
        sort(names.begin(),names.end());
        for(int i = 0;i<names.size();i++){
            std::cout<<names[i]<<"\n";
        }
    }
    else if (command == "write-tree")
    {
        std::string tree_hash = write_tree(".");
        if (tree_hash.empty())
        {
            std::cerr << "Error in writing tree object\n";
            return EXIT_FAILURE;
        }
        std::cout << tree_hash << "\n";
    }
    else if(command == "commit-tree")
    {
        if(argc<5)
        {
            std::cerr << "Less than 5 argument "<< '\n';
            return EXIT_FAILURE;
        }
        string treeSha=argv[2];
        string parentSha="";
        string commitMsg="defult";
        string flagg=argv[3];
        if(flagg=="-p")
        {
            parentSha=argv[4];
            string flagg2=argv[5];
            if(flagg2=="-m")
            {
                commitMsg=argv[6];
            }
            else
            {
                std::cerr <<"Pls provide m "<< '\n';
                return EXIT_FAILURE;
            }
        }
        else if(flagg=="-m")
        {
            commitMsg=argv[4];
        }
        else
        {
            std::cerr <<"No such flag "<< '\n';
            return EXIT_FAILURE;
        }
        commit_tree(treeSha,parentSha,commitMsg);
    }
    else if (command == "clone") {
        if (argc < 3) {
            std::cerr << "No repository provided.\n";
            return EXIT_FAILURE;
        }
        std::string url = argv[2];
        std::string directory = argv[3];
        if (clone(url, directory) != EXIT_SUCCESS) {
            std::cerr << "Failed to clone repository.\n";
            return EXIT_FAILURE;
        }
    }
    else 
    {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
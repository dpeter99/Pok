/****************************
 * Pok Package Manager
 * Client
 * C++ Rewrite
 * 
 * The Chroma Subsystem #15
 * 
 * (c) Gemwire 2021
 * - Curle
 */


#include <iostream>
#include <inc/json.h>
using json = nlohmann::json;
#include <inc/http.h>
#include <sys/stat.h>
#include <fstream>
#include <windows.h>

constexpr unsigned int hash(const char *s, int off = 0) {                        
    return !s[off] ? 5381 : (hash(s, off+1)*33) ^ s[off];                           
}  
static void recursedir(const char *dir) {
    char tmp[256];
    char *p = NULL;
    size_t len;
    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
            tmp[len - 1] = 0;
    for(p = tmp + 1; *p; p++)
            if(*p == '/') {
                    *p = 0;
                    mkdir(tmp);
                    *p = '/';
            }
    mkdir(tmp);
}

int main(int argc, char* argv[]) {
    using namespace http;

    std::string host = "localhost";
    int port = 15060;
    std::string path = "/";
    std::string destPath = "./downloads/";

    std::ofstream file;

    switch(hash(argv[1])) {
        case hash("search"):
            if(argc > 2)
                path.append(argv[2]);
            else {
                std::cout << "Search for what?\r\n";
                return 1;
            }

            break;
        case hash("who-owns"):
            break;
        case hash("list"):
            if(argc > 2)
                path.append(argv[2]).append("/versions");
            else {
                std::cout << "List what?\r\n";
                return 2;
            }

            break;
        
        default:
            if(argc > 2) {
                path.append(argv[1]).append("/").append(argv[2]).append("/archive");
                destPath.append(argv[1]).append("/").append(argv[2]);
            } else {
                path.append(argv[1]).append("/archive");
                destPath.append(argv[1]).append("/latest");
            }

            std::cout << "Downloading " << argv[1] << " to " << destPath << std::endl;

            // mkdir -p <folder>
            recursedir(destPath.substr(0, destPath.find_last_of('/')).c_str());
            
            // touch <file>
            file.open(destPath);

            break;

    };

    try {
        Request request(host.append(":").append(std::to_string(port)).append(path));

        const Response response = request.send("GET");
        std::string contentType;

        for(auto h : response.headers) {
            if(h.find_first_of("Content-Type: ") == 0)
                contentType = h.substr(strlen("Content-Type: "));
        };
        
        if(response.body.size() > 0) {
            if(contentType == "application/json") {
                const std::string resBody(response.body.begin(), response.body.end());
                json jsonResponse = json::parse(resBody);
                std::cout << "Found package " << jsonResponse["name"] << ". Details: " << std::endl;
                for (auto& el : jsonResponse.items()) {
                    std::cout << el.key() << " : " << el.value() << "\n";
                }
            } else if(contentType == "application/octet-stream") {
                for (const auto &e : response.body) file << e;
            }
        } else {
            puts("Server returned bad response");
        }
    } catch (const std::exception &e) {
        std::cerr << "failure " << e.what() << "\n";
    }
}
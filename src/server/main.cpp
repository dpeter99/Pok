/****************************
 * Pok Package Manager
 * Server Endpoint
 * C++ Rewrite
 * 
 * The Chroma Subsystem #15
 * 
 * (c) Gemwire 2021
 * - Curle
 */

#define OPENSSL_API_1_1
#include <inc/civetserver.hpp>
#include <inc/json.h>
using json = nlohmann::json;

#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <csignal>
#include <fstream>
#include <cmath>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

bool enabled = true;

bool shouldRun() {
    return enabled;
}

void shutdown(int sig) {
    printf(" \x1b[32m✔ Shutting down \x1b[0m\n");
    mg_exit_library();
    enabled = false;
}

class ResponseHelper {
    private: 
        int status_code;
        std::string content_type;
        bool _keepAlive;

    public:
        ResponseHelper() {
            status_code = 127001;
            content_type = "dead";
            _keepAlive = false;
        };

        ResponseHelper& setStatusCode(int code) {
            status_code = code;
            return *this;
        };

        ResponseHelper& setContentType(std::string type) {
            content_type = type;
            return *this;
        };

        ResponseHelper& keepAlive() { _keepAlive = true; return *this; };

        void sendHeader(mg_connection* connect) {
            if (status_code == 127001 || content_type == "dead") {
                printf("   \x1b[33m✖ Invalid state! Construct the response first.\r\n");
            }

            std::string response = "HTTP/1.1 ";
            std::string status = status_code == 200 ? "200 OK\r\n" :
                                 status_code == 404 ? "404 NOT FOUND\r\n" :
                                 std::to_string(status_code).append("\r\n");
            response.append(status);

            response.append("Content-Type: ").append(content_type).append("\r\n");

            if(_keepAlive)
                response.append("Connection: close\r\n");

            response.append("\r\n");

            mg_printf(connect, response.c_str());
        };    
};

class PokHandler : public CivetHandler {
    private:
        const std::string searchPrefix = "/search/";
        const std::string versionsPostfix = "/versions";
        const std::string archivePostfix = "/archive";            
        //Load the meta json
        std::ifstream metaFile;

        ResponseHelper header;

        static std::string GetURI(const struct mg_request_info* request_info) {
            const char* uri = request_info->request_uri;
            std::string s(uri);
            return s;
        };

        static std::string GetConnectionSource(const struct mg_request_info* request_info) {
            const char* addr = request_info->remote_addr;
            std::string s(addr);
            return s;
        };

        static std::vector<std::string> listDir(const char* path) {
            std::vector<std::string> entries;

            struct dirent *entry;
            DIR *dir = opendir(path);
            if (dir == NULL) {
                return entries;
            }

            while ((entry = readdir(dir)) != NULL) {
                entries.emplace_back(entry->d_name);
            }

            closedir(dir);

            return entries;
        };

    public:
        json packages;
        PokHandler() {
            metaFile.open("../../packages/meta.json");
            if(!metaFile) {
                std::cerr << "Unable to open package data: " << strerror(errno) <<  std::endl;
                shutdown(2);
            } else {
                metaFile >> packages;
            }
        }

        bool handleGet(CivetServer* server, struct mg_connection* connection) {
            const struct mg_request_info* req = mg_get_request_info(connection);
            const std::string url = req->request_uri;
            std::cout << " \x1b[36m=> Incoming request from " << PokHandler::GetConnectionSource(req) << " for path " << PokHandler::GetURI(req) << std::endl;
            
            //helper.setStatusCode(200).setContentType("text/plain");
            //std::string resStr("You have reached the Gemwire Pok Thing That I Made For The Purpose Of Existing");

            std::string queryResult(
                url.find(searchPrefix) == 0 ? searchFor(url.substr(8)) // startsWith("/search/");
                : url.rfind(versionsPostfix) == (url.size() - versionsPostfix.size()) ? fetchVersions(url.substr(1, url.size() - 9)) // endsWith("/versions");
                : url.rfind(archivePostfix) == (url.size() - archivePostfix.size()) ? "" // endsWith("/archive");)
                : retrieveMeta(url.substr(1), true)); // Default to just printing information

            // We can't printf a file, so we have to special-case for the archive
            if(url.rfind(archivePostfix) == (url.size() - archivePostfix.size())) { // endsWith("/archive");)
                // We need to fetch the archive and set the header info before we send the header
                std::string target(fetchArchive(url.substr(1, url.find_last_of('/') + 1))); // second character up to the last / (packagename/version/)
                header.sendHeader(connection);
                if(!target.compare("")) {
                    // file_body takes a CString representing the file path
                    mg_send_file_body(connection, target.c_str());
                }
            } else {
                // If we're not looking for an archive, send the header and printf the content
                header.sendHeader(connection);
                mg_printf(connection, queryResult.c_str());

            }

            return true;
        };      

        std::string retrieveMeta(std::string item, bool send) {

            if(!metaFile)
                return R"(
{
    "error": 2,
    "meaning": "Package Meta Corrupt",
    "versions": "N/A"
}
                )";

            if(item.find_first_of('/') > -1) { // we have extra
                // do we start with a /?
                if(item.find_first_of('/') == 0)
                    item = item.substr(1);
                
                // do we have a / after this?
                if(item.find_first_of('/') > 0)
                    item = item.substr(0, item.find_first_of('/'));
            }
            std::cout << "   \x1b[36m=> Searching for " << item << " in package repository" << std::endl;

            // Find the item
            for(auto& temp : packages) {
                for(auto& element : temp.items()) {
                    auto val = element.value();
                    if(val["name"].get<std::string>() == item) {
                        std::cout << "     \x1b[36m=> Found element " << val["name"].get<std::string>() << std::endl;
                        if(send) header.setStatusCode(200).setContentType("application/json");
                        return element.value().dump();
               
                    }
                }
            }

            std::cout << "     \x1b[36m=> Package not found." << std::endl;
            header.setStatusCode(404).setContentType("application/json");

            return R"(
                {
                    "error": 1,
                    "meaning": "Package Not Found",
                    "versions": "N/A"
                }
            )";


        }

        std::string searchFor(std::string item) {
            return retrieveMeta(item, true);
        };

        std::string fetchVersions(std::string item) {
            return retrieveMeta(item, true);
        };

        std::string fetchArchive(std::string item) {
            const auto package = item.substr(0, item.find_first_of('/'));
            auto version = item.substr(item.find_first_of('/') + 1);
            // Retrieve package meta for versioning
            json meta = json::parse(retrieveMeta(package, false));
            // Short-circuit on error
            if(meta.contains("error")) return "";
            // If latest or none, get the highest version
            if(version.compare("latest/") || version.compare("")) {
                auto metaVersions = meta["versions"];
                version = metaVersions[metaVersions.size() - 1].dump();
            }

            
            // List the files in /packages/package/version/
            std::string fileToRead;
            std::string targetFolder = "../../packages/";
            targetFolder.append(package).append("/").append(version);
            
            std::cout << "   \x1b[36m=> Preparing to serve " << package << "/" << version << " from " << targetFolder <<  std::endl;
            std::vector<std::string> names(listDir(targetFolder.c_str()));
            if(names.size() > 1) {} // TODO: zip?
            fileToRead = targetFolder.append("/").append(names.at(2)); // . .. <folder contents>
            std::cout << "   \x1b[36m=> Serving " << fileToRead << std::endl;
            header.setStatusCode(200).setContentType("application/octet-stream");
            return fileToRead;
        };
};

int main(void) {


    mg_init_library(0);

    std::vector<std::string> opts = { "document_root", "/web", "listening_ports", "15060" };

    CivetServer server(opts);

    std::cout << "---\n\n\x1b[7m.:Gemwire Pok Server:.\x1b[0m\n" << std::endl;
    std::cout << " \x1b[36m=> Starting up..\n" << std::endl;

    PokHandler handler_reqs;
    server.addHandler("", handler_reqs); // Redirect /** to handler_reqs

    signal(SIGINT, shutdown);
    

    while(shouldRun()) {
        Sleep(1000);
    };

    return 0;

}

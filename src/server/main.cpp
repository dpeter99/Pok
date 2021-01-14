<<<<<<< HEAD
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
#include <inc/CivetServer.h>
#include <inc/json.h>
using json = nlohmann::json;

#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cmath>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif


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
                wprintf(L"   \x1b[33m✖ Invalid state! Construct the response first.");
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
        const std::wstring searchPrefix = L"/search/";
        const std::wstring versionsPostfix = L"/versions";
        const std::wstring archivePostfix = L"/archive";
        ResponseHelper header;

        static std::wstring GetURI(const struct mg_request_info* request_info) {
            const char* uri = request_info->request_uri;
            std::wstring s(charArrToWstring(uri));
            return s;
        };

        static std::wstring GetConnectionSource(const struct mg_request_info* request_info) {
            const char* addr = request_info->remote_addr;
            std::wstring s(charArrToWstring(addr));
            return s;
        };

        static std::wstring charArrToWstring(const char* in) {
            std::string s(in);
            std::wstring wide(s.begin(), s.end());
            return wide;
        }

        static const char* wstringToCharArr(std::wstring in) {
            std::string temp(in.begin(), in.end());
            return temp.c_str();
        }

        static const std::wstring stringToWstring(std::string in) {
            return std::wstring(in.begin(), in.end());
        }

        static const std::string wstringToString(std::wstring in) {
            return std::string(in.begin(), in.end());
        }

        static std::vector<std::wstring> listDir(const char* path) {
            std::vector<std::wstring> entries;

            struct dirent *entry;
            DIR *dir = opendir(path);
            if (dir == NULL) {
                return entries;
            }

            while ((entry = readdir(dir)) != NULL) {
                entries.emplace_back(charArrToWstring(entry->d_name));
            }

            closedir(dir);

            return entries;
        };

    public:
        PokHandler() {
        }

        bool handleGet(CivetServer* server, struct mg_connection* connection) {
            const struct mg_request_info* req = mg_get_request_info(connection);
            const std::wstring url = charArrToWstring(req->request_uri);
            std::wcout << L" \x1b[36m➡ Incoming request from " << PokHandler::GetConnectionSource(req) << " for path " << PokHandler::GetURI(req) << std::endl;
            
            //helper.setStatusCode(200).setContentType("text/plain");
            //std::string resStr("You have reached the Gemwire Pok Thing That I Made For The Purpose Of Existing");

            std::wstring queryResult(
                url.find(searchPrefix) == 0 ? searchFor(url.substr(8)) // startsWith("/search/");
                : url.rfind(versionsPostfix) == (url.size() - versionsPostfix.size()) ? fetchVersions(url.substr(1, url.size() - 9)) // endsWith("/versions");
                : retrieveMeta(url.substr(1), true)); // Default to just printing information

            // We can't printf a file, so we have to special-case for the archive
            if(url.rfind(archivePostfix) == (url.size() - archivePostfix.size())) {
                // We need to fetch the archive and set the header info before we send the header
                std::wstring target(fetchArchive(url.substr(1, url.size() - 8))); // endsWith("/archive");)
                header.sendHeader(connection);
                if(!target.compare(L"")) {
                    // file_body takes a CString representing the file path
                    mg_send_file_body(connection, wstringToCharArr(target));
                }
            } else {
                // If we're not looking for an archive, send the header and printf the content
                header.sendHeader(connection);
                mg_printf(connection, wstringToCharArr(queryResult));

            }

            return true;
        };      

        std::wstring retrieveMeta(std::wstring item, bool send) {
            std::wcout << L"   \x1b[36m➡ Searching for " << item << " in package repository" << std::endl;
            
            //Load the meta json
            std::ifstream metaFile("packages/meta.json");
            json packages;
            metaFile >> packages;

            // Find the item
            for(auto& temp : packages) {
                for(auto& element : temp.items()) {
                    auto val = element.value();
                    if(stringToWstring(val["name"].get<std::string>()) == item) {
                        std::wcout << L"     \x1b[36m➡ Found element " << stringToWstring(val["name"].get<std::string>()) << std::endl;
                        if(send) header.setStatusCode(200).setContentType("application/json");
                        return stringToWstring(element.value().dump());
               
                    }
                }
            }

            std::wcout << L"     \x1b[36m➡ Package not found." << std::endl;
            header.setStatusCode(404).setContentType("application/json");

            return std::wstring(LR"(
                {
                    "error": 1,
                    "meaning": "Package Not Found",
                    "versions": "N/A"
                }
            )");


        }

        std::wstring searchFor(std::wstring item) {
            return retrieveMeta(item, true);
        };

        std::wstring fetchVersions(std::wstring item) {
            return retrieveMeta(item, true);
        };

        std::wstring fetchArchive(std::wstring item) {
            const auto package = item.substr(0, item.find_first_of('/'));
            auto version = item.substr(item.find_first_of('/') + 1);
            // Retrieve package meta for versioning
            json meta = json::parse(retrieveMeta(package, false));
            // Short-circuit on error
            if(meta.contains("error")) return L"";
            // If latest or none, get the highest version
            if(version.compare(L"latest/") || version.compare(L"")) {
                auto metaVersions = meta["versions"];
                std::string strDump(metaVersions[metaVersions.size() - 1].dump());
                std::wstring wideDump(strDump.begin(), strDump.end());
                version = wideDump;
            }
            
            // List the files in /packages/package/version/
            std::wstring fileToRead;
            std::wstring targetFolder = L"/packages/";
            targetFolder.append(package).append(L"/").append(version);
            std::vector<std::wstring> names(listDir(wstringToCharArr(targetFolder)));
            if(names.size() > 1) {} // TODO: zip?

            fileToRead = targetFolder.append(L"/").append(names.at(0));
            std::wcout << L"   \x1b[36m➡ Serving " << fileToRead << std::endl;
            header.setStatusCode(200).setContentType("application/octet-stream");
            return fileToRead;
        };
};

int main(void) {
    _setmode(_fileno(stdout), _O_U16TEXT);
    mg_init_library(0);

    std::vector<std::string> opts = { "document_root", "/web", "listening_ports", "15060" };

    CivetServer server(opts);

    wprintf(L"---\n\n\x1b[7m.:Gemwire Pok Server:.\x1b[0m\n");
    wprintf(L" \x1b[36m➡ Starting up..\n");

    PokHandler handler_reqs;
    server.addHandler("", handler_reqs); // Redirect /** to handler_reqs

    while(
    #ifdef _WIN32
        GetAsyncKeyState(VK_ESCAPE)==0
    #else    
        0
    #endif
        ) {
        Sleep(1000);
    }

    wprintf(L" \x1b[32m✔ Shutting down \x1b[0m\n");
    mg_exit_library();

    return 0;

}
=======
>>>>>>> parent of 68b315c... Client & Server, now talking to each other.

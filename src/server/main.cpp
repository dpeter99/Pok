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
#include <fcntl.h>
#include <stdio.h>
#include <iostream>

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

    public:
        bool handleGet(CivetServer* server, struct mg_connection* connection) {
            const struct mg_request_info* request = mg_get_request_info(connection);

            std::wcout << L" \x1b[36m➡ Incoming request from " << PokHandler::GetConnectionSource(request) << " for path " << PokHandler::GetURI(request) << std::endl;
            
            ResponseHelper helper;

            helper.setStatusCode(200).setContentType("text/plain");

            std::string resStr("You have reached the Gemwire Pok Thing That I Made For The Purpose Of Existing");


            helper.sendHeader(connection);

            mg_printf(connection, resStr.c_str());

            return true;
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

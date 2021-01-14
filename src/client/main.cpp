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
#include <inc/http.h>
#include <windows.h>

int_fast32_t main(void) {
    using namespace http;

    try {
        Request request("localhost:15060/");

        const Response response = request.send("GET");

        if(response.body.size() > 0) {
            const std::string resBody(response.body.begin(), response.body.end());
            puts(resBody.c_str());
        } else {
            puts("Server returned bad response");
        }
    } catch (const std::exception &e) {
        std::cerr << "failure " << e.what() << "\n";
    }
}
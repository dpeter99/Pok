
/****************************
 * Pok Package Manager
 * Server Endpoint
 * 
 * The Chroma Subsystem #15
 * 
 * (c) Gemwire 2021
 * - Curle
 */

const http = require("http");
const fs = require("fs");
const metaJSON = require("./packages/meta.json");

console.log("---\n\n\x1b[7m.:Gemwire Pok Server:.\x1b[0m");
console.log(" \x1b[36m➡ Starting up..");

http.createServer(function (req, result) {
    console.log(" \x1b[36m➡ Incoming request from " + req.socket.remoteAddress + " for path " + req.url);
    
    var queryResult;

    queryResult = req.url.startsWith("/search/") ? searchFor(req.url.substr(8), req, result)
                : req.url.endsWith("/versions")  ? fetchVersions(req.url.substr(1, req.url.length - 9), req, result)
                : req.url.endsWith("/archive")   ? fetchArchive(req.url.substr(1, req.url.length - 8), req, result)
                : retrieveMeta(req.url.substr(1), req, result);

    if(queryResult)
        result.write(queryResult);
    
    result.end();
}).listen(15060);


function retrieveMeta(string, req, result) {
    console.log("   \x1b[36m➡ Searching for " + string + " in package repository");
    var searchResult = metaJSON.packages.find(el => el.name == string);

    if(searchResult) {
        console.log("     \x1b[36m➡ Found element " + searchResult.name);
        result.writeHead(200, "Found meta for result", { "Content-Type": "application/json" });
        return JSON.stringify(searchResult);
    } else {
        console.log("\x1b[36m➡ Not found");
        result.writeHead(404, { "Content-Type": "application/json" });

        result.write(JSON.stringify({
            error: 1,
            meaning: "Package Not Found",
            versions: "N/A"
        }));

        return null;

    }
}

function searchFor(string, req, result) {
    return retrieveMeta(string, req, result);
}

function fetchVersions(string, req, result) {
    return retrieveMeta(string, req, result).versions;

}

function fetchArchive(string, request, result) {
    const package = string.substr(0, string.indexOf("/")); // start to the first slash
    var version = string.substr(string.indexOf("/") + 1);    // first slash to the end
    const packageMeta = metaJSON.packages.find(el => el.name == package);
    if(version == "latest/" || version == "") // If we're latest, get the last version
        version = packageMeta.versions[packageMeta.versions.length - 1];

    var fileToRead = "";

    var filenames = fs.readdirSync("./packages/" + package + "/" + version); // we're reading files here
    if(filenames.length > 1) {}
        // noop
    fileToRead = "./packages/" + package + "/" + version + "/" + filenames[0];
    console.log("   \x1b[36m➡ Serving " + fileToRead);

    var archive = fs.readFileSync(fileToRead);


    result.writeHead(200, "Found archive", { "Content-Type": "application/octet-stream" });
    result.write(archive);

    return null;

}
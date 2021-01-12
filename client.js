/****************************
 * Pok Package Manager
 * Client
 * 
 * The Chroma Subsystem #15
 * 
 * (c) Gemwire 2021
 * - Curle
 */

const http = require("http");
const mkdirp = require("mkdirp");
const fs = require("fs");

const args = process.argv.slice(2);

var activity;
var file, filePath;

var req_opts = { hostname: "localhost", port: 15060, path: "/", method: "GET"}

switch(args[0]) {
    case "search":
        req_opts.path += args[1];
        activity = "searching " + args[1];
        break;
    case "who-owns":
        break;
    case "list":
        req_opts.path += args[1] + "/versions";
        activity = "listing " + args[1];
        break;
    default:

        if(args.length > 1) {
            req_opts.path += args[0] + "/" + args[1] + "/archive";
            filePath = "./downloads/" + args[0] + "/" + args[1] + ""
            activity += "downloading " + args[0] + " version " + args[1];
        } else {
            req_opts.path += args[0] + "/archive";
            filePath = "./downloads/" + args[0] + "/latest";
            activity = "downloading " + args[0];
        }
        console.log(filePath);
        fs.mkdirSync(filePath.substr(0, filePath.lastIndexOf("/")), {recursive:true}); // make sure the intermediary folders exist
        fs.closeSync(fs.openSync(filePath, 'w')); // make sure the file exists

        file = fs.createWriteStream(filePath);
        break;
};

http.get(req_opts, callback => {
    var data = [];
    if(callback.headers["content-type"] == "application/octet-stream")
        callback.pipe(file);

    callback.on("data", dataIn => {
        if(callback.headers["content-type"] == "application/json")
            data.push(dataIn);
    });

    callback.on("end", () => {
        if(callback.headers["content-type"] == "application/json")
            console.dir(JSON.parse(data.toString()), { depth: null, colors: true});
        
        if(file)
            file.close();
    })

}).end();
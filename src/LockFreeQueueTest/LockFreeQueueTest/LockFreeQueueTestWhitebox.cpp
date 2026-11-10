#include "LockFreeQueueTestWhitebox.h"
#include <iostream>
#include <stdexcept>

std::string LockFreeQueueTestWhitebox::strip(const std::string& text)
{
    std::vector<std::string> lines = LockFreeQueueTestTool::split(text, '\n');
    std::string output;
    int32_t depth = 0;
    bool production = false;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::string directive = lines[i];
        size_t start = directive.find_first_not_of(" \t\r");
        directive = start == std::string::npos ? "" : directive.substr(start);
        size_t end = directive.find_last_not_of(" \t\r");
        if (end != std::string::npos)
        {
            directive.resize(end + 1);
        }
        if (!depth && directive == "#ifdef LOCK_FREE_QUEUE_TESTING")
        {
            depth = 1;
            production = false;
            continue;
        }
        if (depth)
        {
            if (directive.compare(0, 4, "#if ") == 0 || directive.compare(0, 7, "#ifdef ") == 0 || directive.compare(0, 8, "#ifndef ") == 0)
            {
                ++depth;
            }
            else if (directive == "#endif")
            {
                if (!--depth)
                {
                    continue;
                }
            }
            else if (depth == 1 && directive == "#else")
            {
                production = true;
                continue;
            }
            if (!production)
            {
                continue;
            }
        }
        output += lines[i];
        if (i + 1 < lines.size())
        {
            output += '\n';
        }
    }
    if (depth)
    {
        throw std::runtime_error("Unclosed testing conditional");
    }
    return output;
}

Json::Value LockFreeQueueTestWhitebox::prepare(const LockFreeQueueTestTool& tool, const std::string& input, const std::string& destination)
{
    typedef LockFreeQueueTestTool IO;
    std::string source = IO::path(input), output = IO::path(destination, false);
    IO::inside(output, IO::join(tool.root(), "src/temp/LockFreeQueueTest"));
    if (source == output)
    {
        throw std::runtime_error("Whitebox output equals source");
    }
    std::string patchFile = IO::join(tool.tests(), "whitebox-patches.json");
    Json::Value configuration = IO::load(patchFile), manifest;
    if (configuration["format"].asInt() != 1 || !configuration["files"].isObject())
    {
        throw std::runtime_error("Unknown whitebox patch format");
    }
    std::map<std::string, std::string> prepared;
    std::vector<std::string> paths = IO::sources(source);
    for (size_t i = 0; i < paths.size(); ++i)
    {
        std::string file = IO::name(paths[i]), raw = IO::read(paths[i]), plain = IO::plain(raw), instrumented = plain;
        if (plain.find("LOCK_FREE_QUEUE_TESTING") != std::string::npos || plain.find("LockFreeQueueTestHook") != std::string::npos)
        {
            throw std::runtime_error("Production contains whitebox code: " + file);
        }
        const Json::Value& changes = configuration["files"][file];
        for (Json::Value::UInt p = 0; p < changes.size(); ++p)
        {
            instrumented = IO::replaceOnce(instrumented, changes[p]["before"].asString(), changes[p]["after"].asString());
        }
        if (strip(instrumented) != plain)
        {
            throw std::runtime_error("Patch changes production semantics: " + file);
        }
        prepared[file] = IO::compiled(instrumented);
        manifest["files"][file]["production_sha256"] = IO::digest(raw);
        manifest["files"][file]["instrumented_sha256"] = IO::digest(prepared[file]);
    }
    std::vector<std::string> patched = configuration["files"].getMemberNames();
    for (size_t i = 0; i < patched.size(); ++i)
    {
        if (!prepared.count(patched[i]))
        {
            throw std::runtime_error("Missing patch source: " + patched[i]);
        }
    }
    if (IO::exists(output))
    {
        std::vector<std::string> old = IO::sources(output);
        for (size_t i = 0; i < old.size(); ++i)
        {
            if (!prepared.count(IO::name(old[i])))
            {
                throw std::runtime_error("Stale whitebox source; use fresh output: " + old[i]);
            }
        }
    }
    for (std::map<std::string, std::string>::const_iterator it = prepared.begin(); it != prepared.end(); ++it)
    {
        std::string file = IO::join(output, it->first);
        if (!IO::exists(file) || IO::read(file) != it->second)
        {
            IO::write(file, it->second);
        }
    }
    manifest["patch_sha256"] = IO::digest(IO::read(patchFile));
    manifest["source"] = source;
    IO::save(IO::join(output, "whitebox-manifest.json"), manifest);
    std::cout << "WHITEBOX " << prepared.size() << " source files; stripping hooks reproduces production exactly" << std::endl;
    return manifest;
}
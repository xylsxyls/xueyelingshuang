#include "NetTestBuild.h"
#include "NetTestProcess.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

bool NetTestBuild::compile(const std::string& executable, const std::vector<std::string>& arguments, const std::string& log)
{
    NetTestProcess process;
    int32_t code = -1;
    const bool finished = process.start(executable, arguments, log) && process.wait(180000, code) == 1 && code == 0;
#ifdef _WIN32
    return finished && process.waitTree(1000) == 1;
#else
    return finished;
#endif
}

bool NetTestBuild::signatures(const std::string& header, std::string& result)
{
    std::vector<uint8_t> bytes;
    if (!CSystem::readBinaryFile(CStringManager::Utf8ToUnicode(header), 1024 * 1024, bytes))
    {
        return false;
    }
    try
    {
        const std::string text(bytes.begin(), bytes.end());
        const std::regex declaration("EMSCRIPTEN_KEEPALIVE\\s+(\\w+)\\s+(\\w+)\\(([^()]*)\\)");
        std::string pending("[");
        std::set<std::string> names;
        for (std::sregex_iterator item(text.begin(), text.end(), declaration), end; item != end; ++item)
        {
            const std::string type = (*item)[1], name = (*item)[2], parameters = (*item)[3];
            if (!names.insert(name).second || (type != "void" && type != "int32_t" && type != "uint32_t"))
            {
                return false;
            }
            std::string signature("_");
            std::istringstream input(parameters);
            std::string parameter;
            while (std::getline(input, parameter, ','))
            {
                const size_t first = parameter.find_first_not_of(" \t\r\n");
                if (first == std::string::npos)
                {
                    continue;
                }
                const size_t last = parameter.find_last_not_of(" \t\r\n");
                if (parameter.substr(first, last - first + 1) == "void")
                {
                    continue;
                }
                signature += parameter.find('*') != std::string::npos || parameter.find("Handle") != std::string::npos ? 'p' : '_';
            }
            if (names.size() != 1)
            {
                pending += ',';
            }
            pending += "\"" + name + ":" + signature + "\"";
        }
        if (names.empty())
        {
            return false;
        }
        pending += ']';
        result.swap(pending);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

int NetTestBuild::run(int argc, char** argv)
{
    std::vector<std::string> raw;
    if (!CSystem::commandArguments(argc, argv, raw) || raw.empty() || raw[0] != "--build-tests")
    {
        return 2;
    }
    const std::set<std::string> keys = { "--target", "--bits", "--sdk", "--headers", "--emscripten", "--python", "--repository", "--output" };
    std::map<std::string, std::string> options;
    for (size_t index = 1; index < raw.size(); ++index)
    {
        const size_t split = raw[index].find('=');
        if (split == std::string::npos || split + 1 == raw[index].size())
        {
            return 2;
        }
        const std::string key = raw[index].substr(0, split), value = raw[index].substr(split + 1);
        if (!keys.count(key) || options.count(key))
        {
            return 2;
        }
        options[key] = key == "--target" || key == "--bits" ? value : CSystem::canonicalPathUtf8(value, false);
        if (options[key].empty())
        {
            return 2;
        }
    }
    if (options.size() != keys.size() || (options["--target"] != "protocol" && options["--target"] != "wasm-tests") ||
        (options["--bits"] != "32" && options["--bits"] != "64"))
    {
        return 2;
    }
    const bool protocol = options["--target"] == "protocol";
    const std::string bits = options["--bits"], sdk = options["--sdk"], root = options["--repository"];
    const std::string headers = options["--headers"];
    const std::string source = root + "/src/NetGatewayTest/NetGatewayTest", output = options["--output"];
    const std::string compiler = options["--emscripten"], python = options["--python"];
    std::vector<std::string> libraries = { "NetInterface.a", "CEncodeDecode.a", "ProtoMessageManager.a", "Variant.a", "Compress.a",
        "NetChannel.a", "libzstd.a", "libprotobuf-lite.a", "libmbedtls.a" };
    if (!protocol)
    {
        for (const std::string& library : { "NetPeer.a", "CSystem.a", "LockFreeHashMap.a", "LockFreeQueue.a" })
        {
            libraries.push_back(library);
        }
    }
    for (const std::string& library : libraries)
    {
        if (!CSystem::fileExists(CStringManager::Utf8ToUnicode(sdk + "/lib/" + library)))
        {
            std::fprintf(stderr, "Prepare the matching Release Wasm archive first: %s\n", library.c_str());
            return 2;
        }
    }
    for (const std::string& file : { python, compiler + "/em++.py", compiler + "/emcc.py", headers + "/NetInterface/NetInterfaceAPI.h" })
    {
        if (!CSystem::fileExists(CStringManager::Utf8ToUnicode(file)))
        {
            return 2;
        }
    }
#ifdef _WIN32
    if (!CreateDirectoryW(CStringManager::Utf8ToUnicode(output).c_str(), nullptr))
    {
        return 2;
    }
#else
    if (mkdir(output.c_str(), 0700) != 0)
    {
        return 2;
    }
#endif
    std::vector<std::string> sources;
    if (protocol)
    {
        sources.push_back("NetProtocolTestBindings.cpp");
    }
    else
    {
        sources = { "NetGatewayTest.cpp", "NetGatewaySuiteCases.cpp", "NetInterfaceCppProbe.cpp", "NetSenderProviderCProbe.c",
            "NetInterfaceTlsTestCases.cpp", "NetCertificateBufferTest.cpp", "NetDirectoryValueTest.cpp",
            "NetTransportContractTest.cpp", "NetInterfaceTransportTest.cpp", "NetGatewayHistoryTest.cpp",
            "../../NetServerManager/NetServerManager/NetGatewayPeerHistory.cpp",
            "NetInterfaceProtoTestCases.cpp", "NetInterfaceConnectionTestCases.cpp", "NetInterfaceConnectionTestCasesHelper.cpp",
            "NetInterfaceHandshakeTestCases.cpp", "NetInterfaceRequestsTestCases.cpp", "NetInterfaceRpcTestCases.cpp",
            "NetIdentityTestCases.cpp", "NetTunnelTestCases.cpp", "NetLinkTestCases.cpp", "NetTestTlsCertificates.c" };
    }
    std::vector<std::string> common = { "-O2", "-g1", "-fexceptions", "-DSTATIC_LIB", "-sMEMORY64=" + std::string(bits == "64" ? "1" : "0"),
        "-iquote", sdk + "/include", "-I" + sdk + "/include/protobuf", "-I" + sdk + "/include/mbedtls",
        "-iquote", headers, "-iquote", source };
    std::vector<std::string> objects;
    for (size_t index = 0; index < sources.size(); ++index)
    {
        const bool c = sources[index].size() >= 2 && sources[index].substr(sources[index].size() - 2) == ".c";
        const std::string object = output + "/unit" + std::to_string(index) + ".o";
        std::vector<std::string> arguments = { compiler + (c ? "/emcc.py" : "/em++.py"), c ? "-std=c11" : "-std=c++11", "-c", source + "/" + sources[index], "-o", object };
        arguments.insert(arguments.end(), common.begin(), common.end());
        std::printf("TEST_BUILD %s\n", sources[index].c_str());
        std::fflush(stdout);
        if (!compile(python, arguments, output + "/compile" + std::to_string(index) + ".log"))
        {
            return 1;
        }
        objects.push_back(object);
    }
    const std::string name = protocol ? "NetProtocolTest" : "NetGatewayWasmTest";
    std::vector<std::string> arguments = { compiler + "/em++.py" };
    arguments.insert(arguments.end(), common.begin(), common.end());
    arguments.insert(arguments.end(), objects.begin(), objects.end());
    arguments.push_back("-Wl,--start-group");
    for (const std::string& library : libraries)
    {
        arguments.push_back(sdk + "/lib/" + library);
    }
    arguments.push_back("-Wl,--end-group");
    for (const std::string& flag : { "-sMODULARIZE=1", "-sENVIRONMENT=node", "-sALLOW_MEMORY_GROWTH=1",
        "-sMAXIMUM_MEMORY=268435456", "-sSTACK_SIZE=1048576", "-sWASM_BIGINT=1" })
    {
        arguments.push_back(flag);
    }
    arguments.push_back("-sEXPORT_NAME=create" + name);
    if (protocol)
    {
        arguments.push_back("--no-entry");
        arguments.push_back("-sFILESYSTEM=0");
        arguments.push_back("-sEXPORTED_FUNCTIONS=[\"_malloc\",\"_free\"]");
        if (bits == "64")
        {
            std::string conversions;
            if (!signatures(source + "/NetProtocolTestBindings.h", conversions))
            {
                return 1;
            }
            std::ofstream file((output + "/conversions.json").c_str(), std::ios::binary);
            file << conversions;
            file.close();
            if (!file)
            {
                return 1;
            }
            arguments.push_back("-sSIGNATURE_CONVERSIONS=@" + output + "/conversions.json");
        }
    }
    else
    {
        arguments.push_back("-sEXPORTED_RUNTIME_METHODS=[\"FS\",\"callMain\"]");
    }
    arguments.push_back("-o");
    arguments.push_back(output + "/" + name + ".js");
    if (!compile(python, arguments, output + "/link.log"))
    {
        return 1;
    }
    std::ofstream module((output + "/" + name + ".js").c_str(), std::ios::binary | std::ios::app);
    module << "\nmodule.exports.sdk = { bits: " << bits << " };\n";
    module.close();
    if (!module)
    {
        return 1;
    }
    std::printf("TEST_BUILD_PASS %s bits=%s\n", name.c_str(), bits.c_str());
    return 0;
}
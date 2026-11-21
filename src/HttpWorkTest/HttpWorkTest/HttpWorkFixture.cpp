#include "HttpFixtureOrigin.h"
#include "PlatformTest/PlatformTestHost.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

/** 执行测试程序或证书工具；选项作为独立参数传递，输出和失败状态交给 CTest
*/
static void Check(PlatformTestHost& host, std::vector<std::string> command, const std::vector<std::string>& extra, int seconds)
{
    command.insert(command.end(), extra.begin(), extra.end());
    std::string output;
    host.command(command, seconds, output);
    std::cout << output;
}

/** 为本次 TLS 测试生成临时根证书及仅用于 localhost 的叶证书，目录由宿主对象回收
*/
static void Certificates(PlatformTestHost& host, const std::string& openssl)
{
    const std::string dir = host.directory() + "/";
    std::string output;
    PlatformTestHost::write(dir + "ext", "subjectAltName=DNS:localhost\nextendedKeyUsage=serverAuth\nbasicConstraints=CA:FALSE\n");
    host.command({openssl, "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "2", "-subj", "/CN=HTTP fixture root", "-addext", "basicConstraints=critical,CA:TRUE", "-keyout", dir + "ca.key", "-out", dir + "ca.pem"}, 30, output);
    host.command({openssl, "req", "-newkey", "rsa:2048", "-nodes", "-subj", "/CN=localhost", "-keyout", dir + "leaf.key", "-out", dir + "leaf.csr"}, 30, output);
    host.command({openssl, "x509", "-req", "-in", dir + "leaf.csr", "-CA", dir + "ca.pem", "-CAkey", dir + "ca.key", "-set_serial", "1", "-days", "1", "-extfile", dir + "ext", "-out", dir + "leaf.pem"}, 30, output);
}

/** 在本机或 Node 下检查 HTTP 和 TLS 信任、主机名及认证前不发送正文
*/
static void RunClient(PlatformTestHost& host, const std::vector<std::string>& client, const std::string& openssl, bool node)
{
    {
        HttpFixtureOrigin plain(0, "", "");
        const std::string origin = "http://127.0.0.1:" + std::to_string(plain.port());
        {
            HttpFixtureOrigin blocked(0, "", "");
            Check(host, client, {"--limits", "http://127.0.0.1:" + std::to_string(blocked.port())}, 70);
        }
        const char* capacities[] = {"--capacity-default", "--capacity-unlimited"};
        for (const char* mode : capacities)
        {
            HttpFixtureOrigin blocked(0, "", "");
            Check(host, client, {mode, "http://127.0.0.1:" + std::to_string(blocked.port())}, 70);
        }
        Check(host, client, {"--client", origin}, 70);
        if (!node)
        {
            HttpFixtureOrigin other(0, "", "");
            Check(host, client, {"--reuse", origin, "http://127.0.0.1:" + std::to_string(other.port())}, 70);
        }
    }
    if (openssl.empty())
    {
        std::cout << "[SKIP] TLS certificate cases require an OpenSSL command-line executable\n";
        return;
    }
    Certificates(host, openssl);
    const std::string dir = host.directory() + "/";
    HttpFixtureOrigin secure(0, host.read(dir + "leaf.pem"), host.read(dir + "leaf.key"));
    const std::string port = std::to_string(secure.port());
    const std::string url = "https://localhost:" + port;
    Check(host, client, {"--reject", url}, 70);
    std::vector<std::string> trusted;
    if (node)
    {
        trusted = {PLATFORM_TEST_CMAKE, "-E", "env", "NODE_EXTRA_CA_CERTS=" + dir + "ca.pem"};
    }
    trusted.insert(trusted.end(), client.begin(), client.end());
    std::vector<std::string> reject = {"--reject", "https://127.0.0.1:" + port};
    std::vector<std::string> success = {"--client", url};
    if (!node)
    {
        reject.push_back(dir + "ca.pem");
        success.push_back(dir + "ca.pem");
    }
    Check(host, trusted, reject, 70);
    if (secure.posts() != 0)
    {
        throw std::runtime_error("Request body reached the server before TLS authentication");
    }
    Check(host, trusted, success, 70);
    if (!node)
    {
        HttpFixtureOrigin other(0, "", "");
        const unsigned posts = secure.posts();
        std::string output;
        host.command({openssl, "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "2", "-subj",
            "/CN=Independent HTTP fixture root", "-addext", "basicConstraints=critical,CA:TRUE", "-keyout", dir + "other.key",
            "-out", dir + "other.pem"}, 30, output);
        Check(host, client, {"--reuse", url, "http://127.0.0.1:" + std::to_string(other.port()), dir + "ca.pem", dir + "other.pem"}, 70);
        Check(host, client, {"--uncached-ca", url, "http://127.0.0.1:" + std::to_string(other.port()), dir + "ca.pem", dir + "other.pem"}, 70);
        Check(host, client, {"--large-ca-default", url, dir + "ca.pem"}, 70);
        Check(host, client, {"--large-ca-unlimited", url, dir + "ca.pem"}, 70);
        if (secure.posts() != posts + 2)
        {
            throw std::runtime_error("Rejected private/system trust switch delivered its POST");
        }
    }
}

int main(int argc, char** argv)
{
    try
    {
        if (argc > 1 && (std::string(argv[1]) == "device" || std::string(argv[1]) == "device-benchmark"))
        {
            if (argc < 5)
            {
                throw std::runtime_error("device <runner> <platform> <product> [runner options]");
            }
            std::string platform(argv[3]);
            if (platform == "ios-device")
            {
                std::cerr << "Set HTTPWORK_TEST_ORIGIN to a fixture reachable by the physical iOS device\n";
                return 2;
            }
            if (platform != "ios-simulator" && platform != "android")
            {
                throw std::runtime_error("Unknown fixture device " + platform);
            }
            PlatformTestHost host;
            HttpFixtureOrigin origin(0, "", "");
            std::vector<std::string> command(argv + 2, argv + argc);
            const std::string port = std::to_string(origin.port());
            if (platform == "android")
            {
                command.push_back("--http-loopback");
                command.push_back(port);
            }
            Check(host, command, {"--", std::string(argv[1]) == "device" ? "--client" : "--benchmark", "http://127.0.0.1:" + port}, 210);
            return 0;
        }
        std::string openssl;
        bool node = false, serve = false, benchmark = false;
        unsigned port = 0;
        int first = 1;
        for (; first < argc; ++first)
        {
            std::string option(argv[first]);
            if (option == "--serve")
            {
                serve = true;
            }
            else if (option == "--benchmark")
            {
                benchmark = true;
            }
            else if (option == "--node-ca")
            {
                node = true;
            }
            else if (option == "--openssl" && first + 1 < argc)
            {
                openssl = argv[++first];
            }
            else if (option == "--port" && first + 1 < argc)
            {
                std::string number(argv[++first]);
                if (number.empty() || number.size() > 5 || number.find_first_not_of("0123456789") != std::string::npos || std::atoi(number.c_str()) > 65535)
                {
                    throw std::runtime_error("Invalid HTTP fixture port");
                }
                port = static_cast<unsigned>(std::atoi(number.c_str()));
            }
            else
            {
                break;
            }
        }
        if (serve)
        {
            HttpFixtureOrigin origin(port, "", "");
            std::cout << "http://127.0.0.1:" << origin.port() << std::endl;
            for (;;)
            {
                std::this_thread::sleep_for(std::chrono::hours(1));
            }
        }
        if (first >= argc)
        {
            throw std::runtime_error("HttpWorkFixture [--openssl executable] [--node-ca] <client command> | --serve [--port number]");
        }
        PlatformTestHost host;
        std::vector<std::string> client(argv + first, argv + argc);
        if (benchmark)
        {
            HttpFixtureOrigin origin(0, "", "");
            Check(host, client, {"--benchmark", "http://127.0.0.1:" + std::to_string(origin.port())}, 70);
        }
        else
        {
            RunClient(host, client, openssl, node);
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
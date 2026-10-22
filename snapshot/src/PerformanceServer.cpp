#include "PerformanceServer.h"

// Must include httplib before any JUCE includes that might conflict
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include "../libs/httplib/httplib.h"

struct PerformanceServer::Impl
{
    httplib::Server server;
};

PerformanceServer::PerformanceServer(Listener& l, int p)
    : juce::Thread("PerformanceServer"), listener(l), port(p),
      impl(std::make_unique<Impl>())
{
}

PerformanceServer::~PerformanceServer()
{
    stopServer();
}

void PerformanceServer::setWebRoot(const juce::File& directory)
{
    webRoot = directory;
}

void PerformanceServer::startServer()
{
    if (isThreadRunning())
        return;

    startThread();
}

void PerformanceServer::stopServer()
{
    if (impl)
        impl->server.stop();

    stopThread(2000);
}

void PerformanceServer::run()
{
    auto& server = impl->server;

    // GET /api/presets
    server.Get("/api/presets", [this](const httplib::Request&, httplib::Response& res) {
        auto names = listener.getPresetNames();
        juce::String json = "[";
        for (int i = 0; i < names.size(); ++i)
        {
            if (i > 0) json += ",";
            json += "\"" + names[i].replace("\"", "\\\"") + "\"";
        }
        json += "]";
        res.set_content(json.toStdString(), "application/json");
    });

    // POST /api/presets/:name/activate
    server.Post(R"(/api/presets/(.+)/activate)", [this](const httplib::Request& req, httplib::Response& res) {
        auto name = juce::String(req.matches[1].str());

        // Use a WaitableEvent to synchronize with the message thread
        juce::WaitableEvent done;
        bool success = false;

        juce::MessageManager::callAsync([this, name, &done, &success]() {
            success = listener.activatePreset(name);
            done.signal();
        });

        done.wait(5000);

        if (success)
            res.set_content("{\"ok\":true}", "application/json");
        else
        {
            res.status = 404;
            res.set_content("{\"error\":\"preset not found\"}", "application/json");
        }
    });

    // GET /api/irs
    server.Get("/api/irs", [this](const httplib::Request&, httplib::Response& res) {
        auto names = listener.getIRNames();
        juce::String json = "[";
        for (int i = 0; i < names.size(); ++i)
        {
            if (i > 0) json += ",";
            json += "\"" + names[i].replace("\"", "\\\"") + "\"";
        }
        json += "]";
        res.set_content(json.toStdString(), "application/json");
    });

    // POST /api/irs/:name/activate
    server.Post(R"(/api/irs/(.+)/activate)", [this](const httplib::Request& req, httplib::Response& res) {
        auto name = juce::String(req.matches[1].str());

        juce::WaitableEvent done;
        bool success = false;

        juce::MessageManager::callAsync([this, name, &done, &success]() {
            success = listener.activateIR(name);
            done.signal();
        });

        done.wait(5000);

        if (success)
            res.set_content("{\"ok\":true}", "application/json");
        else
        {
            res.status = 404;
            res.set_content("{\"error\":\"IR not found\"}", "application/json");
        }
    });

    // GET /api/status
    server.Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
        auto presetName = listener.getCurrentPresetName();
        auto irName = listener.getCurrentIRName();

        juce::String json = "{";
        json += "\"preset\":\"" + presetName.replace("\"", "\\\"") + "\",";
        json += "\"ir\":\"" + irName.replace("\"", "\\\"") + "\"";
        json += "}";
        res.set_content(json.toStdString(), "application/json");
    });

    // Static file serving
    if (webRoot.exists())
    {
        auto webRootPath = webRoot.getFullPathName().toStdString();
        server.set_mount_point("/", webRootPath);
    }

    server.listen("0.0.0.0", port);
}

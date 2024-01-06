#pragma once

#include <string>
#include <list>
#include <mutex>

#define CURL_STATICLIB 1
#include <curl/curl.h>
#include <json/json.h>

#include <types/Pilot.h>

namespace vacdm::com
{
  // handles all communication with the backend API
  class Server
  {
  public:
    typedef struct ServerConfiguration
    {
      std::string name = "";
      bool allowMasterInSweatbox = false;
      bool allowMasterAsObserver = false;

      std::string versionFull = "";
      std::int64_t versionMajor = -1;
      std::int64_t versionMinor = -1;
      std::int64_t versionPatch = -1;

      std::list<std::string> supportedAirports;
    } ServerConfiguration_t;

  private:
    struct Communication
    {
      std::mutex lock;
      CURL *socket;

      Communication() : lock(), socket(curl_easy_init()) {}
    };

    Communication m_getRequest;
    Communication m_postRequest;
    Communication m_patchRequest;
    Communication m_deleteRequest;

    bool m_apiIsChecked;
    bool m_apiIsValid;
    std::string m_baseUrl;
    bool m_clientIsMaster;
    std::string m_errorCode;
    ServerConfiguration m_serverConfiguration;

    Server();

    void sendPostMessage(const std::string &endpointUrl, const Json::Value &root);
    void sendPatchMessage(const std::string &endpointUrl, const Json::Value &root);

  public:
    ~Server();
    Server(const Server &) = delete;
    Server(Server &&) = delete;

    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    static Server &instance();

    void changeServerAddress(const std::string &url);
    bool checkWebApi();
    const ServerConfiguration_t getServerConfig() const;
    std::list<types::Pilot> getFlightsOfAirport(const std::string &airport);

    // messages to backend | DPI = Departure Planing Information
    void postInitialPilotData(const types::Pilot &data);
    void sendTargetDpiNow(const types::Pilot &data);
    void sendTargetDpiTarget(const types::Pilot &data);
    void sendTargetDpiSequenced(const types::Pilot &data);
    void sendAtcDpi(const types::Pilot &data);
    void sendCustomDpiTaxioutTime(const types::Pilot &data);
    void sendCustomDpiRequest(const types::Pilot &data);

    const std::string &errorMessage() const;
    void setMaster(bool master);
  };
}
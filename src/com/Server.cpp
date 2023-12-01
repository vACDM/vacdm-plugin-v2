#include "Server.h"

#include "Version.h"
#include "utils/date.h"

using namespace vacdm::com;

static std::string __receivedDeleteData;
static std::string __receivedGetData;
static std::string __receivedPatchData;
static std::string __receivedPostData;

static std::size_t receiveCurlDelete(void *ptr, std::size_t size,
                                     std::size_t nmemb, void *stream)
{
  (void)stream;

  std::string serverResult = static_cast<char *>(ptr);
  __receivedDeleteData += serverResult;
  return size * nmemb;
}

static std::size_t receiveCurlGet(void *ptr, std::size_t size,
                                  std::size_t nmemb, void *stream)
{
  (void)stream;

  std::string serverResult = static_cast<char *>(ptr);
  __receivedGetData += serverResult;
  return size * nmemb;
}

static std::size_t receiveCurlPatch(void *ptr, std::size_t size,
                                    std::size_t nmemb, void *stream)
{
  (void)stream;

  std::string serverResult = static_cast<char *>(ptr);
  __receivedPatchData += serverResult;
  return size * nmemb;
}

static std::size_t receiveCurlPost(void *ptr, std::size_t size,
                                   std::size_t nmemb, void *stream)
{
  (void)stream;

  std::string serverResult = static_cast<char *>(ptr);
  __receivedPostData += serverResult;
  return size * nmemb;
}

Server::Server() : m_getRequest(),
                   m_postRequest(),
                   m_patchRequest(),
                   m_deleteRequest(),
                   m_apiIsChecked(false),
                   m_apiIsValid(false),
                   m_baseUrl("https://vacdm-dev.vatsim-germany.org"),
                   m_clientIsMaster(false),
                   m_errorCode()
{
  /* configure the get request */
  curl_easy_setopt(m_getRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(m_getRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(m_getRequest.socket, CURLOPT_HTTP_VERSION,
                   static_cast<long>(CURL_HTTP_VERSION_1_1));
  curl_easy_setopt(m_getRequest.socket, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(m_getRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurlGet);
  curl_easy_setopt(m_getRequest.socket, CURLOPT_TIMEOUT, 2L);

  /* configure the post request */
  curl_easy_setopt(m_postRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(m_postRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(m_postRequest.socket, CURLOPT_HTTP_VERSION,
                   static_cast<long>(CURL_HTTP_VERSION_1_1));
  curl_easy_setopt(m_postRequest.socket, CURLOPT_WRITEFUNCTION,
                   receiveCurlPost);
  curl_easy_setopt(m_postRequest.socket, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt(m_postRequest.socket, CURLOPT_VERBOSE, 1);
  // TODO: define token
  // struct curl_slist *headers = nullptr;
  // headers = curl_slist_append(headers, "Accept: application/json");
  // headers = curl_slist_append(headers, ("Authorization: Bearer " +
  // this->m_authToken).c_str()); headers = curl_slist_append(headers,
  // "Content-Type: application/json"); curl_easy_setopt(m_postRequest.socket,
  // CURLOPT_HTTPHEADER, headers);

  /* configure the patch request */
  curl_easy_setopt(m_patchRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(m_patchRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(m_patchRequest.socket, CURLOPT_HTTP_VERSION,
                   static_cast<long>(CURL_HTTP_VERSION_1_1));
  curl_easy_setopt(m_patchRequest.socket, CURLOPT_WRITEFUNCTION,
                   receiveCurlPatch);
  curl_easy_setopt(m_patchRequest.socket, CURLOPT_CUSTOMREQUEST, "PATCH");
  curl_easy_setopt(m_patchRequest.socket, CURLOPT_VERBOSE, 1);
  // curl_easy_setopt(m_patchRequest.socket, CURLOPT_HTTPHEADER, headers);

  /* configure the delete request */
  curl_easy_setopt(m_deleteRequest.socket, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(m_deleteRequest.socket, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(m_deleteRequest.socket, CURLOPT_HTTP_VERSION,
                   static_cast<long>(CURL_HTTP_VERSION_1_1));
  curl_easy_setopt(m_deleteRequest.socket, CURLOPT_CUSTOMREQUEST, "DELETE");
  curl_easy_setopt(m_deleteRequest.socket, CURLOPT_WRITEFUNCTION,
                   receiveCurlDelete);
  curl_easy_setopt(m_deleteRequest.socket, CURLOPT_TIMEOUT, 2L);
}

Server::~Server()
{
  if (nullptr != m_getRequest.socket)
  {
    std::lock_guard guard(m_getRequest.lock);
    curl_easy_cleanup(m_getRequest.socket);
    m_getRequest.socket = nullptr;
  }

  if (nullptr != m_postRequest.socket)
  {
    std::lock_guard guard(m_postRequest.lock);
    curl_easy_cleanup(m_postRequest.socket);
    m_postRequest.socket = nullptr;
  }

  if (nullptr != m_patchRequest.socket)
  {
    std::lock_guard guard(m_patchRequest.lock);
    curl_easy_cleanup(m_patchRequest.socket);
    m_patchRequest.socket = nullptr;
  }

  if (nullptr != m_deleteRequest.socket)
  {
    std::lock_guard guard(m_deleteRequest.lock);
    curl_easy_cleanup(m_deleteRequest.socket);
    m_deleteRequest.socket = nullptr;
  }
}

Server &Server::instance()
{
  static Server __instance;
  return __instance;
}

const std::string &Server::errorMessage() const { return m_errorCode; }

const Server::ServerConfiguration_t Server::getServerConfig() const
{
  return this->m_serverConfiguration;
}

void Server::changeServerAddress(const std::string &url)
{
  this->m_baseUrl = url;
  this->m_apiIsChecked = false;
  this->m_apiIsValid = false;
}

bool Server::checkWebApi()
{
  if (this->m_apiIsChecked == true)
    return this->m_apiIsValid;

  std::lock_guard guard(m_getRequest.lock);
  if (m_getRequest.socket == nullptr)
    return this->m_apiIsValid;

  __receivedGetData.clear();

  std::string url = m_baseUrl + "/api/v1/config/plugin";
  curl_easy_setopt(m_getRequest.socket, CURLOPT_URL, url.c_str());

  // send the get request
  CURLcode result = curl_easy_perform(m_getRequest.socket);
  if (result != CURLE_OK)
    return this->m_apiIsValid;

  Json::CharReaderBuilder builder{};
  auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
  std::string errors;
  Json::Value root;

  if (reader->parse(__receivedGetData.c_str(), __receivedGetData.c_str() + __receivedGetData.length(), &root, &errors))
  {
    // check version of plugin matches required version from backend
    Json::Value configJson = root["version"];
    int majorVersion = configJson.get("major", Json::Value(-1)).asInt();
    if (PLUGIN_VERSION_MAJOR != majorVersion)
    {
      if (majorVersion == -1)
      {
        this->m_errorCode = "Could not find required major version, confirm the server URL.";
      }
      else
      {
        this->m_errorCode = "Backend-version is incompatible. Please update the plugin. ";
        this->m_errorCode += "Server requires version " + std::to_string(majorVersion) + ".X.X. ";
        this->m_errorCode += "You are using version " + std::string(PLUGIN_VERSION);
      }
      this->m_apiIsValid = false;
    }
    else
    {
      this->m_apiIsValid = true;

      // set config parameters
      ServerConfiguration config;

      Json::Value versionObject = root["version"];
      config.versionFull = versionObject["version"].asString();
      config.versionMajor = versionObject["major"].asInt();
      config.versionMinor = versionObject["minor"].asInt();
      config.versionPatch = versionObject["patch"].asInt();

      Json::Value configObject = root["config"];
      config.name = configObject["serverName"].asString();
      config.allowMasterInSweatbox = configObject["allowSimSession"].asBool();
      config.allowMasterAsObserver = configObject["allowObsMaster"].asBool();

      // get supported airports from backend
      Json::Value airportsJsonArray = root["supportedAirports"];
      if (!airportsJsonArray.empty())
      {
        for (size_t i = 0; i < airportsJsonArray.size(); i++)
        {
          const std::string airport = airportsJsonArray[i].asString();

          config.supportedAirports.push_back(airport);
        }
      }
      this->m_serverConfiguration = config;
    }
  }
  else
  {
    this->m_errorCode = "Invalid backend-version response: " + __receivedGetData;
    this->m_apiIsValid = false;
  }
  m_apiIsChecked = true;
  return this->m_apiIsValid;
}

/// @brief Sends a post message to the specififed endpoint url with the root as content
/// @param endpointUrl endpoint url to send the request to
/// @param root message content
void Server::sendPostMessage(const std::string &endpointUrl, const Json::Value &root)
{
  if (this->m_apiIsChecked == false || this->m_apiIsValid == false || this->m_clientIsMaster == false)
    return;

  Json::StreamWriterBuilder builder{};
  const auto message = Json::writeString(builder, root);

  std::lock_guard guard(this->m_postRequest.lock);
  if (m_postRequest.socket != nullptr)
  {
    std::string url = m_baseUrl + endpointUrl;
    curl_easy_setopt(m_postRequest.socket, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_postRequest.socket, CURLOPT_POSTFIELDS, message.c_str());

    curl_easy_perform(m_postRequest.socket);
  }
}

/// @brief Sends a patch message to the specified endpoint url with the root as content
/// @param endpointUrl endpoint url to send the request to
/// @param root message content
void Server::sendPatchMessage(const std::string &endpointUrl, const Json::Value &root)
{
  if (this->m_apiIsChecked == false || this->m_apiIsValid == false || this->m_clientIsMaster == false)
    return;

  Json::StreamWriterBuilder builder{};
  const auto message = Json::writeString(builder, root);

  std::lock_guard guard(this->m_patchRequest.lock);
  if (m_patchRequest.socket != nullptr)
  {
    std::string url = m_baseUrl + endpointUrl;
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_POSTFIELDS, message.c_str());

    curl_easy_perform(m_patchRequest.socket);
  }
}

void Server::postInitialPilotData(const types::Pilot &data)
{
  Json::Value message;

  message["callsign"] = data.callsign;
  message["inactive"] = false;

  message["position"] = Json::Value();
  message["position"]["lat"] = data.latitude;
  message["position"]["lon"] = data.longitude;

  message["flightplan"] = Json::Value();
  message["flightplan"]["departure"] = data.origin;
  message["flightplan"]["arrival"] = data.destination;

  message["vacdm"] = Json::Value();
  message["vacdm"]["eobt"] = vacdm::utils::timestampToIsoString(data.eobt);
  message["vacdm"]["tobt"] = vacdm::utils::timestampToIsoString(data.eobt);

  message["clearance"] = Json::Value();
  message["clearance"]["dep_rwy"] = data.runway;
  message["clearance"]["sid"] = data.sid;
  message["clearance"]["inital_climb"] = data.initialClimb;
  message["clearance"]["assigned_squawk"] = data.assignedSquawk;
  message["clearance"]["current_squawk"] = data.currentSquawk;

  this->sendPostMessage("/api/v1/pilots", message);
}

void Server::sendTargetDpiNow(const types::Pilot &data)
{
  Json::Value message;

  message["callsign"] = data.callsign;
  message["message_type"] = "T-DPI-n";
  message["asat"] = vacdm::utils::timestampToIsoString(data.asat);

  this->sendPatchMessage("/api/v1/messages/t-dpi-n", message);
}

void Server::sendTargetDpiTarget(const types::Pilot &data)
{
  Json::Value message;

  message["callsign"] = data.callsign;
  message["message_type"] = "T-DPI-t";
  message["tobt_state"] = "CONFIRMED";
  message["tobt"] = vacdm::utils::timestampToIsoString(data.tobt);

  this->sendPatchMessage("/api/v1/messages/t-dpi-t", message);
}

void Server::sendTargetDpiSequenced(const types::Pilot &data)
{
  Json::Value message;

  message["callsign"] = data.callsign;
  message["message_type"] = "T-DPI-s";
  message["asat"] = vacdm::utils::timestampToIsoString(data.asat);

  this->sendPatchMessage("/api/v1/messages/t-dpi-s", message);
}

void Server::sendAtcDpi(const types::Pilot &data)
{
  Json::Value message;

  message["callsign"] = data.callsign;
  message["message_type"] = "A-DPI";
  message["aobt"] = vacdm::utils::timestampToIsoString(data.aobt);

  this->sendPatchMessage("/api/v1/messages/a-dpi", message);
}

void Server::sendCustomDpiTaxioutTime(const types::Pilot &data)
{
  Json::Value message;

  message["callsign"] = data.callsign;
  message["message_type"] = "X-DPI-taxi";
  message["exot"] = std::chrono::duration_cast<std::chrono::minutes>(data.exot.time_since_epoch()).count();

  this->sendPatchMessage("/api/v1/messages/x-dpi-t", message);
}

void Server::sendCustomDpiRequest(const types::Pilot &data)
{
  Json::Value message;

  message["callsign"] = data.callsign;
  message["message_type"] = "X-DPI-req";
  message["asrt"] = vacdm::utils::timestampToIsoString(data.asrt);
  message["aort"] = vacdm::utils::timestampToIsoString(data.aort);

  this->sendPatchMessage("/api/v1/messages/x-dpi-r", message);
}
#include <stdlib.h>
#include <string.h>

#include <concord/discord.h>
#include <concord/websockets.h>

#include <coglink/lavalink-internal.h>
#include <coglink/lavalink.h>
#include <coglink/definitions.h>

size_t __coglink_WriteMemoryCallback(void *data, size_t size, size_t nmemb, void *userp) {
  size_t writeSize = size * nmemb;
  struct coglink_requestInformation *mem = userp;

  char *ptr = realloc(mem->body, mem->size + writeSize + 1);
  if (!ptr) {
    perror("[SYSTEM] Not enough memory to realloc.\n");
    return 1;
  }

  mem->body = ptr;
  memcpy(&(mem->body[mem->size]), data, writeSize);
  mem->size += writeSize;
  mem->body[mem->size] = 0;

  return writeSize;
}

size_t __coglink_WriteMemoryCallbackNoSave(void *data, size_t size, size_t nmemb, void *userp) {
  (void) data; (void) size; (void) userp;
  return nmemb;
}

int __coglink_checkCurlCommand(struct coglink_lavaInfo *lavaInfo, CURL *curl, CURLcode cRes, char *pos, int additionalDebugging, int getResponse, struct coglink_requestInformation *res) {
  if (cRes != CURLE_OK) {
    if (lavaInfo->debugging->allDebugging || additionalDebugging || lavaInfo->debugging->curlErrorsDebugging) log_fatal("[coglink:libcurl] curl_easy_setopt [%s] failed: %s\n", pos, curl_easy_strerror(cRes));

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    if (getResponse) free(res->body);

    return COGLINK_LIBCURL_FAILED_SETOPT;
  }
  return COGLINK_SUCCESS;
}

int __coglink_performRequest(struct coglink_lavaInfo *lavaInfo, struct coglink_requestInformation *res, struct __coglink_requestConfig *config) {
  if (!config->usedCURL) curl_global_init(CURL_GLOBAL_ALL);

  CURL *curl = config->usedCURL;
  if (!config->usedCURL) curl = curl_easy_init();

  if (!curl) {
    if (lavaInfo->debugging->allDebugging || lavaInfo->debugging->curlErrorsDebugging) log_fatal("[coglink:libcurl] Error while initializing libcurl.");

    curl_global_cleanup();

    return COGLINK_LIBCURL_FAILED_INITIALIZE;
  }

  char lavaURL[128 + 12 + 9 + config->pathLength];

  if (config->useVPath) {
    if (lavaInfo->node->ssl) snprintf(lavaURL, sizeof(lavaURL), "https://%s/v4%s", lavaInfo->node->hostname, config->path);
    else snprintf(lavaURL, sizeof(lavaURL), "http://%s/v4%s", lavaInfo->node->hostname, config->path);
  } else {
    if (lavaInfo->node->ssl) snprintf(lavaURL, sizeof(lavaURL), "https://%s%s", lavaInfo->node->hostname, config->path);
    else snprintf(lavaURL, sizeof(lavaURL), "http://%s%s", lavaInfo->node->hostname, config->path);
  }

  CURLcode cRes = curl_easy_setopt(curl, CURLOPT_URL, lavaURL);
  if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "1", config->additionalDebuggingError, config->getResponse, res) != COGLINK_SUCCESS) return COGLINK_LIBCURL_FAILED_SETOPT;

  struct curl_slist *chunk = NULL;
    
  if (lavaInfo->node->password) {
    char AuthorizationH[256];
    snprintf(AuthorizationH, sizeof(AuthorizationH), "Authorization: %s", lavaInfo->node->password);
    chunk = curl_slist_append(chunk, AuthorizationH);

    if (lavaInfo->debugging->allDebugging || config->additionalDebuggingSuccess || lavaInfo->debugging->curlSuccessDebugging) log_debug("[coglink:libcurl] Authorization header set.");
  }

  if (config->body) {
    chunk = curl_slist_append(chunk, "Content-Type: application/json");
    if (lavaInfo->debugging->allDebugging || config->additionalDebuggingSuccess || lavaInfo->debugging->curlSuccessDebugging) log_debug("[coglink:libcurl] Content-Type header set.");
  }

  chunk = curl_slist_append(chunk, "Client-Name: Coglink");
  if (lavaInfo->debugging->allDebugging || config->additionalDebuggingSuccess || lavaInfo->debugging->curlSuccessDebugging) log_debug("[coglink:libcurl] Client-Name header set.");

  chunk = curl_slist_append(chunk, "User-Agent: libcurl");
  if (lavaInfo->debugging->allDebugging || config->additionalDebuggingSuccess || lavaInfo->debugging->curlSuccessDebugging) log_debug("[coglink:libcurl] User-Agent header set.");

  cRes = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "2", config->additionalDebuggingError, config->getResponse, res) != COGLINK_SUCCESS) {
    curl_slist_free_all(chunk);
    return COGLINK_LIBCURL_FAILED_SETOPT;
  }

  if (config->requestType == __COGLINK_DELETE_REQ || config->requestType == __COGLINK_PATCH_REQ) {
    if (config->requestType == __COGLINK_DELETE_REQ) cRes = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    else if (config->requestType == __COGLINK_PATCH_REQ) cRes = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "3", config->additionalDebuggingError, config->getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }
  }

  if (config->getResponse) {
    (*res).body = malloc(1);
    (*res).size = 0;

    cRes = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __coglink_WriteMemoryCallback);

    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "4", config->additionalDebuggingError, config->getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }

    cRes = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&(*res));
    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "5", config->additionalDebuggingError, config->getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }
  } else {
    cRes = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __coglink_WriteMemoryCallbackNoSave);

    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "6", config->additionalDebuggingError, config->getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }
  }

  if (config->body) {
    cRes = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, config->body);
    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "7", config->additionalDebuggingError, config->getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }

    cRes = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, config->bodySize);
    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "8", config->additionalDebuggingError, config->getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }
  }

  cRes = curl_easy_perform(curl);
  if (cRes != CURLE_OK) {
    if (lavaInfo->debugging->allDebugging || config->additionalDebuggingError || lavaInfo->debugging->curlErrorsDebugging) log_fatal("[coglink:libcurl] curl_easy_perform failed: %s\n", curl_easy_strerror(cRes));

    curl_easy_cleanup(curl);
    curl_slist_free_all(chunk);
    curl_global_cleanup();
    if (config->getResponse) free((*res).body);

    return COGLINK_LIBCURL_FAILED_PERFORM;
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(chunk);
  curl_global_cleanup();
  
  if (lavaInfo->debugging->allDebugging || lavaInfo->debugging->curlSuccessDebugging) log_debug("[coglink:libcurl] Performed request successfully.");

  return COGLINK_SUCCESS;
}

int __coglink_checkParse(struct coglink_lavaInfo *lavaInfo, jsmnf_pair *field, char *fieldName) {
  if (!field) {
    if (lavaInfo->debugging->allDebugging || lavaInfo->debugging->checkParseErrorsDebugging) log_error("[coglink:jsmn-find] Failed to find %s field.", fieldName);
    return COGLINK_JSMNF_ERROR_FIND;
  }
  if (lavaInfo->debugging->allDebugging || lavaInfo->debugging->checkParseSuccessDebugging) log_debug("[coglink:jsmn-find] Successfully found %s field.", fieldName);
  return COGLINK_PROCEED;
}

void __coglink_randomString(char *dest, size_t length) {
  char charset[] = "abcdefghijklmnopqrstuvwxyz";

  while(length-- > 0) {
    size_t pos = (double) rand() / RAND_MAX * (sizeof(charset) - 1);
    *dest++ = charset[pos];
  }
  *dest = '\0';
}

int __coglink_IOPoller(struct io_poller *io, CURLM *multi, void *user_data) {
  (void) io; (void) multi;
  struct coglink_lavaInfo *lavaInfo = user_data;
  return !ws_multi_socket_run(lavaInfo->ws, &lavaInfo->tstamp) ? COGLINK_WAIT : COGLINK_SUCCESS;
}

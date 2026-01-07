#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

/* Buffer to store response */
typedef struct {
  char *data;
  size_t size;
} Buffer;

/* Write callback - stores data in buffer */
static size_t write_to_buffer(void *contents, size_t size, size_t nmemb, void *userp) {
  Buffer *buf = (Buffer *)userp;
  size_t realsize = size * nmemb;
  
  char *ptr = realloc(buf->data, buf->size + realsize + 1);
  if (!ptr) return 0;
  
  buf->data = ptr;
  memcpy(&(buf->data[buf->size]), contents, realsize);
  buf->size += realsize;
  buf->data[buf->size] = '\0';
  
  return realsize;
}

/* Setup curl with URL */
static CURL *setup_curl(const char *url, Buffer *buf) {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL *curl = curl_easy_init();
  
  buf->data = NULL;
  buf->size = 0;
  
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
  
  return curl;
}

/* Fetch data */
static void fetch_data(CURL *curl) {
  curl_easy_perform(curl);
}

/* Cleanup */
static void cleanup(CURL *curl, Buffer *buf) {
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  free(buf->data);
}

int main(void) {
  Buffer buf;
  CURL *curl = setup_curl(
    "https://gamma-api.polymarket.com/events?series_id=10188&sportsMarketType=moneyline&active=true&closed=false",
    &buf
  );
  fetch_data(curl);

  FILE *f = fopen("moneyline_markets_epl.json", "w");
  cJSON *json = cJSON_Parse(buf.data);
  if (json) {
    char *pretty = cJSON_Print(json);
    fprintf(f, "%s\n", pretty);
    free(pretty);
    cJSON_Delete(json);
  } else {
    fprintf(f, "%s\n", buf.data);
  }
  fclose(f);
  printf("Saved to moneyline_markets_epl.json\n");

  cleanup(curl, &buf);
  return 0;
}
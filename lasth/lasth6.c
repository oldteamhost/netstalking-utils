/*
 * lasth6.c (LastTrench) - Finding what the person left behind online.
 * You give the software a name and it searches social media pages
 * and websites with that name.
 * 
 * Copyright (c) [2023] [oldteam & lomaster]
 * SPDX-License-Identifier: BSD-3-Clause
 *   Сделано от души 2023.
*/

#include <assert.h>
#include <complex.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define VERSION 6.0
#define RUN_USAGE argv[0]

void usage(const char* run);
void add_name(const char* name);
void add_service(const char* service);
void free_services(void);
void free_names(void);

void logg(const char *path, const char *format, ...);
#include <curl/curl.h>
void readfile(const char* path);
void remove_specials(char* buffer);
int get_response_code(const char* url, int timeout_ms);

const char* short_options = "d:hvt:";
const struct option long_options[] =
{
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'v'},
  {"timeout", required_argument, 0, 't'},
  {"delay", required_argument, 0, 'd'},

  {"txt", required_argument, 0, 1},
  {"html", required_argument, 0, 2},
  {"db", no_argument, 0, 3},
  {"code", required_argument, 0, 4},
  {"base", required_argument, 0, 5},
  {"debug", no_argument, 0, 6},
  {0,0,0,0}
};

struct lasth_options
{
  int timeout_ms;
  int delay_ms;
  const char* txt_path;
  const char* html_path;
  const char* base_path;
#include <stdbool.h>
  bool debug;
  int code;
};

char* tempnames;
char **names = NULL;
int len = 0;

char **services = NULL;
int slen = 0;

int success = 0;

void add_service(const char* service)
{
  for (int i = 0; i < slen; i++) {
    if (services[i] != NULL && strcmp(services[i], service) == 0) {
      return;
    }
  }
  slen++;
  services = (char **)realloc(services, slen * sizeof(char *));
  services[slen - 1] = strdup(service);
}

void free_services(void)
{
  for (int i = 0; i < slen; i++) {
    if (services[i] != NULL)
      free(services[i]);
  }
  if (services != NULL)
    free(services);
}

void add_name(const char* name)
{
  for (int i = 0; i < len; i++) {
    if (names[i] != NULL && strcmp(names[i], name) == 0) {
      return;
    }
  }
  len++;
  names = (char **)realloc(names, len * sizeof(char *));
  names[len - 1] = strdup(name);
}

void free_names(void)
{
  for (int i = 0; i < len; i++) {
    if (names[i] != NULL)
      free(names[i]);
  }
  if (names != NULL)
    free(names);
}

int main(int argc, char** argv)
{
  struct lasth_options lo =
  {
    .timeout_ms = 800,
    .delay_ms = 0,
    .code = 200,
    .txt_path = NULL,
    .html_path = NULL,
    .base_path = NULL,
    .debug = false,
  };

  if (argc <= 1) {
    usage(RUN_USAGE);
    return 0;
  }

  int rez, option_index = 0;
  while ((rez = getopt_long_only(argc, argv, short_options, long_options, &option_index)) != EOF)
  {
    switch (rez)
    {
      case 'h':
        usage(RUN_USAGE);
        return 0;
      case 'v':
        logg(lo.txt_path, "oldteam & lomaster %0.1f\n", VERSION);
        return 0;
      case 't':
        lo.timeout_ms = atoi(optarg);
        break;
      case 'd':
        lo.delay_ms = atoi(optarg);
        break;
      case 1:
        lo.txt_path = optarg;
        break;
      case 2:
        lo.html_path = optarg;
        break;
      case 3:
      case 6:
        lo.debug = true;
        break;
      case 4:
        lo.code = atoi(optarg);
        break;
      case 5:
        lo.base_path = optarg;
        break;
      case '?':
      default:
        usage(RUN_USAGE);
        return 0;
    }
  }

  time_t now = time(NULL);
  struct tm *t = localtime(&now); char datetime[20];
  strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", t);

  logg(lo.txt_path, "Starting LastTrench %0.1f at %s\n", VERSION, datetime);
  if (optind < argc) {
    tempnames = argv[optind];
  }

  readfile(lo.base_path);

  remove_specials(tempnames);
  char *token;
  char *str = strdup(tempnames);
  token = strtok(str, ",");
  while (token != NULL) {
    add_name(token);
    token = strtok(NULL, ",");
  }
  free(str);

  int code;
  for (int i = 0; i < len; i++) {
    if (i > 0){
      logg(lo.txt_path, "\n");
    }
    logg(lo.txt_path, "Checking name %s:\n", names[i]);
    for (int j = 0; j < slen; j++) {
      char* resurl = services[j];
      strcat(resurl, names[i]);
      code = get_response_code(resurl, lo.timeout_ms);
      if (code == lo.code) {
        logg(lo.txt_path, "FOUND: %s%s\n", services[j], names[i]);
        success++;
      }
      else if (code != lo.code && lo.debug) {
        logg(lo.txt_path, "FAILED: %s%s\n", services[j], names[i]);
      }
    }
  }

  logg(lo.txt_path, "\nLastTrench done: %d successful in 0.3s\n", success);

  free_names();
  free_services();
}

void readfile(const char* path)
{
  FILE *file = fopen(path, "r");
  if (!file) {
    return;
  }

  char line[4096];
  while (fgets(line, sizeof(line), file) != NULL) {
    remove_specials(line);
    add_service(line);
  }
  fclose(file);
}

void remove_specials(char* buffer)
{
  int len = strlen(buffer);
  int i, j = 0;

  for (i = 0; i < len; i++) {
      if (buffer[i] != '\r' && buffer[i] != '\n' && buffer[i] != '\t') {
      buffer[j++] = buffer[i];
    }
  }
  buffer[j] = '\0';
}

#include <stdarg.h>
void logg(const char *path, const char *format, ...)
{
  FILE *temp;
  va_list args;

  if (path != NULL) {
    if ((temp = fopen(path, "a")) == NULL){
      perror("logg");
      return;
    }

    va_start(args, format);
    vfprintf(temp, format, args);
    va_end(args);
    fclose(temp);
  }

  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

size_t clearWrite(void *buffer, size_t size, size_t nmemb, void *userp)
{
  return size * nmemb;
}

int get_response_code(const char* url, int timeout_ms)
{
  long response_code = 0;

  CURL *curl = curl_easy_init();
  if (!curl) {
    return -1;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, clearWrite);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_ms);
  CURLcode res = curl_easy_perform(curl);

  if (res == CURLE_OK) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  }

  curl_easy_cleanup(curl);
  return (int)response_code;
}

void usage(const char* run)
{
  puts("LastTrench: Finding what the person left behind online.\n");

  printf("usage: %s <name, [name1].../> [flags]\n\n", run);

  puts("arguments main:");
  puts("  -h, -help            Show this help message and exit.");
  puts("  -v, -version         Display version information and dependencies.");
  puts("  -db, -debug          Saving and outputting even pages that are not working.");
  puts("  -t, -timeout <ms>    Set timeout on response.");
  puts("  -d, -delay <ms>      Set a delay when receiving a page.");

  puts("\narguments save:");
  puts("  -txt <file>          Save output to txt.");
  puts("  -html <path/>        Save output to html.");

  puts("\narguments user:");
  puts("  -base <PATH>         Specify your file with links.");
  puts("  -code <CODE>         Specify your correct answer code.");
}

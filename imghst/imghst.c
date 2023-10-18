/*
 * imghst.c - is a utility for parsing pictures
 * from photohosting sites.
 *
 * Copyright (c) [2023] [oldteam & lomaster]
 * SPDX-License-Identifier: BSD-3-Clause
 *   Сделано от души 2023.
*/

#include <curl/curl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <string.h>

#define VERSION "3.1"
#define MAX_BUF_SIZE 4096

const char* run;

struct grab_options
{
  const char* domain;
  const char* protocol;
  const char* user_agent;

  const char* config_path;
  const char* txt_path;
  const char* _proxy;

  char* path;
  char* format;

  bool preset_info, custom_path,
       debug, proxy, txt;

  int token_length, count, timeout_ms;
};

void      usage(void);
size_t    write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);
char*     generate_random_string(int length);
int       delete_file(const char* path);
int       download_file(const char* node, const char* out_file, const char* user_agent, const char* proxy);
long int  get_file_size(const char* file_name);
void      parse_config(const char* filename, struct grab_options* gopt);
void      write_line_to_file(const char* file_name, const char* line);
void      delay_ms(int milliseconds);

const char* short_options = "hvc:p:";

const struct option long_options[] =
{
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'v'},
  {"count", required_argument, 0, 'c'},
  {"path", required_argument, 0, 'p'},
  {"format", required_argument, 0, 1},
  {"token-length", required_argument, 0, 2},
  {"protocol", required_argument, 0, 3},
  {"user-agent", required_argument, 0, 4},
  {"debug", no_argument, 0, 5},
  {"db", no_argument, 0, 6},
  {"preset", required_argument, 0, 7},
  {"cfg", required_argument, 0, 8},
  {"timeout", required_argument, 0, 9},
  {"txt", required_argument, 0, 10},
  {"proxy", required_argument, 0, 11},
  {"cfg-info", no_argument, 0, 12},
  {0,0,0,0}
};

int main(int argc, char** argv)
{
  struct grab_options gopt =
  {
    .domain = "",
    .protocol = "https://",
    .user_agent = "imghst",
    .path = "",
    .format = ".png",
    .token_length = 5,
    .timeout_ms = 0,
    .count = 10000,

    .config_path = NULL,
    ._proxy = NULL
  };

  run = argv[0];

  if (argc <= 1) {
    usage();
    return 0;
  }

  int rez, option_index = 0;
  while ((rez = getopt_long_only(argc, argv, short_options, long_options, &option_index)) != EOF){
    switch (rez) {
      case 'h':
        usage();
        return 0;
      case 'v':
        printf("lomaster & oldteam %s\n", VERSION);
        break;
      case 'c':
        gopt.count = atoi(optarg);
        break;
      case 'p':
        gopt.custom_path = true;
        gopt.path = optarg;
        break;
      case 1:
        gopt.format = optarg;
        break;
      case 2:
        gopt.token_length = atoi(optarg);
        break;
      case 3:
        if (strcmp(optarg, "http") == 0){
          gopt.protocol = "http://";
        }
        else if (strcmp(optarg, "https") == 0){
          gopt.protocol = "https://";
        }
        else {
          usage();
          return 0;
        }
        break;
      case 4:
        gopt.user_agent = optarg;
        break;
      case 5:
      case 6:
        gopt.debug = true;
        break;
      case 7:
      case 8:
        gopt.config_path = optarg;
        break;
      case 9:
        gopt.timeout_ms = atoi(optarg);
        break;
      case 10:
        gopt.txt = true;
        gopt.txt_path = optarg;
        break;
      case 11:
        gopt.proxy = true;
        gopt._proxy = optarg;
        break;
      case 12:
        gopt.preset_info = true;
        break;
      case '?':
      default:
        usage();
        return 0;
    }
  }

  time_t now = time(NULL);
  struct tm *t = localtime(&now); char datetime[20];
  strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", t);

  printf("Starting Imghst %s at %s\n", VERSION, datetime);

  if (optind < argc) {
    gopt.domain = argv[optind];
  }

  if (gopt.config_path != NULL) {
    parse_config(gopt.config_path, &gopt);
  }

  if (gopt.preset_info) {
    putchar('\n');
    printf("domain: %s\n", gopt.domain);
    printf("protocol: %s\n", gopt.protocol);
    printf("user_agent: %s\n", gopt.user_agent);
    printf("txt_path: %s\n", gopt.txt_path);
    printf("path: %s\n", gopt.path);
    printf("format: %s\n", gopt.format);
    printf("token_length: %d\n", gopt.token_length);
    printf("count: %d\n", gopt.count);
    printf("timeout: %d\n", gopt.timeout_ms);
    printf("debug: %d\n", gopt.debug);
    printf("proxy: %s\n", gopt._proxy);
    putchar('\n');
  }

  puts("PATH\t\tSTATE\tTIME\tURL");

  time_t start_time_full, end_time_full;
  double time_diff_full;

  int success = 0;
  start_time_full = time(NULL);

  int i = 1;
  for (; i <= gopt.count; i++) {
    clock_t start_time, end_time;
    double time_elapsed_ms;

    start_time = clock();
    delay_ms(gopt.timeout_ms);

    char* token = generate_random_string(gopt.token_length);
    char* out_file = (char*) malloc(1000);
    out_file[0] = '\0';

    char* result_url = (char*) malloc(2000);
    result_url[0] = '\0';

    char* file_name = (char*) malloc(1000);
    file_name[0] = '\0';

    strcat(file_name, token);
    strcat(file_name, gopt.format);
    strcat(out_file, gopt.path);

    if (gopt.custom_path){
      strcat(out_file, "/");
    }

    strcat(out_file, file_name);

    strcat(result_url, gopt.protocol);
    strcat(result_url, gopt.domain);
    strcat(result_url, "/");
    strcat(result_url,token);
    strcat(result_url, gopt.format);

    download_file(result_url, out_file, gopt.user_agent, gopt._proxy);

    if (token != NULL) {
      free(token);
    }
    if (file_name != NULL) {
      free(file_name);
    }

    int file_size = get_file_size(out_file);
    int state;

    if (file_size < 100) {
      if (gopt.debug != 1) {
        delete_file(out_file);
      }
      state = EOF;
    }
    else {
      state = 1;
      success++;
    }

    end_time = clock();
    time_elapsed_ms = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000.0;

    if (state == -1 && gopt.debug == 1) {
      if (gopt.txt) {
        write_line_to_file(gopt.txt_path, result_url);
      }
      printf("%s\tfailed\t%dms\t%s\n", out_file, (int)time_elapsed_ms, result_url);
    }
    if (state == 1) {
      if (gopt.txt) {
        write_line_to_file(gopt.txt_path, result_url);
      }
      printf("%s\tappear\t%dms\t%s\n", out_file, (int)time_elapsed_ms, result_url);
    }
    if (result_url != NULL) {
      free(result_url);
    }
    if (out_file != NULL) {
      free(out_file);
    }
  }

  end_time_full = time(NULL);
  time_diff_full = difftime(end_time_full, start_time_full);

  printf("\nImghst done: %d successful in %.1fs\n", success, time_diff_full);
  return 0;
}

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
  return fwrite(ptr, size, nmemb, stream);
}

char *generate_random_string(int len)
{
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  char *str = malloc((len + 1) * sizeof(char));
  int i, index;
  struct timespec ts;
  uint32_t seed;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  seed = (uint32_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
  srand(seed);

  for (i = 0; i < len; i++) {
    index = rand() % (sizeof(charset) - 1);
    str[i] = charset[index];
  }

  str[len] = '\0';
  return str;
}

#include <unistd.h>
int delete_file(const char* path)
{
  int result = 0;
  result = unlink(path);
  return result;
}

int download_file(const char* node, const char* out_file,
    const char* user_agent, const char* proxy)
{
  CURL* curl;
  FILE* fp;
  CURLcode res;

  curl = curl_easy_init();
  if (!curl){
    return 1;
  }

  fp = fopen(out_file, "wb");
  if (!fp) {
    fprintf(stderr, "Failed to open output file.\n");
    goto fail;
  }

  curl_easy_setopt(curl, CURLOPT_URL, node);

  if (proxy != NULL) {
    curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
  }

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    goto fail;
  }

  fclose(fp);
  curl_easy_cleanup(curl);

  return 0;

fail:
  if (!fp){
    fclose(fp);
  }
  curl_easy_cleanup(curl);
  return -1;
}

void write_line_to_file(const char* file_name, const char* line)
{
  FILE* fp = fopen(file_name, "a");
  if (!fp) {
    return;
  }

  fprintf(fp, "%s\n", line);
  fclose(fp);
}

void delay_ms(int milliseconds)
{
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

long int get_file_size(const char* file_name)
{
  FILE* fp = fopen(file_name, "rb");
  long int size;

  if (fp == NULL) {
    return -1;
  }

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);

  fclose(fp);
  return size;
}

void parse_config(const char* filename, struct grab_options* gopt)
{
  FILE* file = fopen(filename, "r");
  char buf[MAX_BUF_SIZE];

  while (fgets(buf, sizeof(buf), file)) {
    buf[strcspn(buf, "\n")] = '\0';

    char* key = strtok(buf, "=");
    char* value = strtok(NULL, "=");

    if (strcmp(key, "domain") == 0) {
      gopt->domain = strdup(value);
    }
    else if (strcmp(key, "protocol") == 0) {
      gopt->protocol = strdup(value);
      if (strcmp(gopt->protocol, "http") == 0){
        gopt->protocol = "http://";
      }
      if (strcmp(gopt->protocol, "https") == 0){
        gopt->protocol = "https://";
      }
    }
    else if (strcmp(key, "user-agent") == 0) {
      gopt->user_agent = strdup(value);
    }
    else if (strcmp(key, "path") == 0) {
      gopt->path = strdup(value);
      if (strcmp(gopt->path, "default") == 0){
        gopt->path = "";
      }
      else {
        strcat(gopt->path, "/");
      }
    }
    else if (strcmp(key, "format") == 0) {
      gopt->format = strdup(value);
    }
    else if (strcmp(key, "token_length") == 0) {
      gopt->token_length = atoi(value);
    }
    else if (strcmp(key, "count") == 0) {
      gopt->count = atoi(value);
    }
    else if (strcmp(key, "debug") == 0) {
      gopt->debug = strcmp(value, "true") == 0 ? 1 : 0;
    }
    else if (strcmp(key, "txt_path") == 0) {
      gopt->txt_path = strdup(value);
      gopt->txt = 1;
      if (strcmp(gopt->txt_path, "false") == 0) {
        gopt->txt = 0;
      }
    }
    else if (strcmp(key, "timeout") == 0) {
      gopt->timeout_ms = atoi(value);
    }
    else if (strcmp(key, "proxy") == 0) {
      gopt->_proxy = strdup(value);
      gopt->proxy = 1;

      if (strcmp(gopt->_proxy, "false") == 0){
        gopt->proxy = 0;
        gopt->_proxy = NULL;
      }
    }
  }
  fclose(file);
}

void usage(void)
{
  puts("Imghst - is a utility for parsing pictures from photohosting sites.\n");
  printf("usage: %s [DOMAIN], [FLAGS]\n\n", run);

  puts("arguments program:");
  puts("  -h, -help            Show this help message and exit.");
  puts("  -v, -version         Display version information and dependencies.");

  puts("arguments main:");
  puts("  -db, -debug          Saving and outputting even links that are not working.");
  puts("  -timeout <ms>        Set delay in ms.");
  puts("  -format <.format>    Set photo format.");
  puts("  -protocol <protocol> Set protocol on connect (http,https).");
  puts("  -token-length <num>  Set the length of the generated token.\n");

  puts("arguments user:");
  puts("  -user-agent <agent>  Set user-agent on connect.");
  puts("  -proxy <p:ip:port>   Using proxy.\n");
  puts("  -cfg-info            Display config.");

  puts("arguments output:");
  puts("  -count <num>         Count photos.");
  puts("  -path <path>         Set the path where the photos will be saved.");
  puts("  -txt <path>          Save links in a text document.");
  puts("  -cfg <path>          Use config for parsing.");
}

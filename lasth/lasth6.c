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
#define RUN_USAGE argv[1]

void usage(const char* run);
void add_name(const char* name);
void add_service(const char* service);
void free_services(void);
void free_names(void);

struct http_header
{
  const char* dest_host;
  const char* method;
  const char* path;
  const char* user_agent;
  const char* content_type;
  unsigned long content_len;
  char* auth_header;
};

void readfile(const char* path);
void remove_specials(char* buffer);
void logg(const char *path, const char *format, ...);
void get_redirect(const char* http_content, char* redirect_buffer, size_t buffer_size);
int send_http_request(const char* ip, const int port, const int timeout_ms,
    const struct http_header* header, char* response_buffer, size_t buffer_size);

char* clean_url(const char* url);
int parse_http_response_code(const char* http_content);
void get_ip(const char* dns, char* ip_buffer, size_t buffer_size);
int get_response_code(const char* url, const char* path, int port, int timeout_ms, struct http_header *hp);

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

const char* tempnames;
char **names = NULL;
int len  = 0;

char **services = NULL;
int slen  = 0;

int success = 0;

void add_service(const char* service)
{
  slen++;
  services = (char **)realloc(services, slen * sizeof(char *));
  services[slen - 1] = strdup(service);
}

void free_services(void)
{
  for (int i = 0; i < slen; i++) {
    free(services[i]);
  }
  free(services);
}

void add_name(const char* name)
{
  len++;
  names = (char **)realloc(names, len * sizeof(char *));
  names[len - 1] = strdup(name);
}

void free_names(void)
{
  for (int i = 0; i < len; i++) {
    free(names[i]);
  }
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

  struct http_header hh =
  {
    .auth_header = "",
    .content_len = 0,
    .content_type = "",
    .method = "GET",
    .user_agent = "lath6",
    .path = "/",
    .dest_host = "",
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
    logg(lo.txt_path, "SERVICE    CODE    STATE    URL\n");
    for (int j = 0; j < slen; j++) {
      code = get_response_code(services[j], names[i], 80, lo.timeout_ms, &hh);
      logg(lo.txt_path, "code = %d\n", code);
      if (code == lo.code){
        logg(lo.txt_path, "Success: %s%s\n", services[j], names[i]);
        success++;
      }
      else {
        logg(lo.txt_path, "fuck: %s%s\n", services[j], names[i]);
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

void get_ip(const char* dns, char* ip_buffer, size_t buffer_size)
{
  struct addrinfo hints;
  struct addrinfo* addrinfo_result;
  struct sockaddr_in* addr;
  const char* ip;
  int res = 0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  res = getaddrinfo(dns, NULL, &hints, &addrinfo_result);
  if (res != 0) {
    strncpy(ip_buffer, "n/a", buffer_size);
    return;
  }

  addr = (struct sockaddr_in*)addrinfo_result->ai_addr;
  ip = inet_ntoa(addr->sin_addr);

  strncpy(ip_buffer, ip, buffer_size);
  ip_buffer[buffer_size - 1] = '\0';

  freeaddrinfo(addrinfo_result);
}

int get_response_code(const char* url, const char* path, int port, int timeout_ms, struct http_header *hp)
{
#define IP_CHAR_SIZE 16
  char response_buffer[4096];
  char redirect_buffer[4096];
  char ipbuf[IP_CHAR_SIZE];
  char* newurl = clean_url(url);
  int send, code;

  hp->dest_host = newurl;
  hp->path = path;

  get_ip(newurl, ipbuf, IP_CHAR_SIZE);
  free(newurl);

  send = send_http_request(ipbuf, port, timeout_ms, hp, response_buffer, 4096);
  if (send == -1) {
    return -1;
  }
  get_redirect(response_buffer, redirect_buffer, 4096);
  printf("redir = %s\n", redirect_buffer);
  if (strcmp(response_buffer, "n/a") != 0) {
    memset(response_buffer, 0, 4096);
    hp->path = response_buffer;
    send = send_http_request(ipbuf, port, timeout_ms, hp, response_buffer, 4096);
  }

  return parse_http_response_code(response_buffer);
}

#include <ctype.h>
char* case_insensitive_strstr(const char* haystack, const char* needle)
{
  while (*haystack) {
    bool match = true;
    for (size_t i = 0; needle[i]; i++) {
      if (tolower(haystack[i]) != tolower(needle[i])) {
        match = false;
        break;
      }
    }
    if (match) {
      return (char*)haystack;
    }
    haystack++;
  }
  return NULL;
}

#define DEFAULT_LOCATION 0
#define CONTENT_LOCATION 1
char* parse_location(const char* http_header, int type_location)
{
  char* location_str;
  if (type_location == DEFAULT_LOCATION){
    location_str = "Location:";
  }
  else if (type_location == CONTENT_LOCATION) {
    location_str = "Content-Location:";
  }

  char* end_str = "\r\n";
  char* result = NULL;

  char* location_it = case_insensitive_strstr(http_header, location_str);
  if (location_it != NULL) {
    location_it += strlen(location_str);
    char* end_it = strstr(location_it, end_str);
    if (end_it != NULL) {
      int length = end_it - location_it;
      result = (char*)malloc(length + 1);
      if (result != NULL) {
        strncpy(result, location_it, length);
        result[length] = '\0';

        char* src = result;
        char* dest = result;
        while (*src) {
          if (!isspace((unsigned char)*src)) {
            *dest = *src;
            dest++;
          }
          src++;
        }
        *dest = '\0';
      }
    }
  }

  return result;
}

char* parse_http_equiv(const char* html)
{
  char* htmll = strdup(html);
  char* meta_str = "<meta";
  char* http_equiv_str = "http-equiv=\"refresh\"";
  char* content_str = "content=\"";
  char* end_str = "\"";
  char* url = NULL;

  for (int i = 0; htmll[i]; i++) {
    htmll[i] = tolower(htmll[i]);
  }

  char* meta_it = strstr(htmll, meta_str);

  if (meta_it != NULL) {
    char* http_equiv_it = strstr(meta_it, http_equiv_str);
    if (http_equiv_it != NULL) {
      char* content_it = strstr(http_equiv_it, content_str);
      if (content_it != NULL) {
        content_it += strlen(content_str);
        char* end_it = strstr(content_it, end_str);
        if (end_it != NULL) {
          int length = end_it - content_it;
          url = (char*)malloc(length + 1);
          if (url != NULL) {
            strncpy(url, content_it, length);
            url[length] = '\0';
            char* src = url;
            char* dest = url;
            while (*src) {
              if (!isspace((unsigned char)*src)) {
                *dest = *src;
                dest++;
              }
              src++;
            }

            *dest = '\0';

            /* top prefixes */
            char* url_prefixes[] = {"0;url=", "1;url=", "2;url=", "0.0;url=", "12;url="};
            for (int i = 0; i < sizeof(url_prefixes) / sizeof(url_prefixes[0]); i++) {
              if (strncmp(url, url_prefixes[i], strlen(url_prefixes[i])) == 0) {
                memmove(url, url + strlen(url_prefixes[i]), strlen(url) - strlen(url_prefixes[i]) + 1);
              }
            }
          }
        }
      }
    }
  }

  free(htmll);
  return url;
}

char* parse_url_from_js(const char* html)
{
  char* search_str = "window.location.href = \"";
  char* end_str = "\"";
  char* result = NULL;
  char* start_pos = strstr(html, search_str);

  if (start_pos != NULL) {
    start_pos += strlen(search_str);
    char* end_pos = strstr(start_pos, end_str);

    if (end_pos != NULL) {
      int length = end_pos - start_pos;
      result = (char*)malloc(length + 1);

      if (result != NULL) {
        strncpy(result, start_pos, length);
        result[length] = '\0';
      }
    }
  }
  return result;
}

void get_redirect(const char* http_content, char* redirect_buffer, size_t buffer_size)
{
  char* content = parse_location(http_content, DEFAULT_LOCATION);
  if (content != NULL) {
    strncpy(redirect_buffer, content, buffer_size - 1);
    redirect_buffer[buffer_size - 1] = '\0';
    free(content);
    return;
  }

  char* content_location = parse_location(http_content, CONTENT_LOCATION);
  if (content_location != NULL) {
    strncpy(redirect_buffer, content_location, buffer_size - 1);
    redirect_buffer[buffer_size - 1] = '\0';
    free(content_location);
    return;
  }

  char* http_equiv = parse_http_equiv(http_content);
  if (http_equiv != NULL) {
    strncpy(redirect_buffer, http_equiv, buffer_size - 1);
    redirect_buffer[buffer_size - 1] = '\0';
    free(http_equiv);
    return;
  }

  char* window_js = parse_url_from_js(http_content);
  if (window_js != NULL) {
    strncpy(redirect_buffer, window_js, buffer_size - 1);
    redirect_buffer[buffer_size - 1] = '\0';
    free(window_js);
    return;
  }

  redirect_buffer[0] = '\0';
}

char* clean_url(const char* url)
{
  const char* https_prefix = "https://";
  const char* http_prefix = "http://";
  const char* www_prefix = "www.";

  if (strncmp(url, https_prefix, strlen(https_prefix)) == 0) {
    url += strlen(https_prefix);
  }
  else if (strncmp(url, http_prefix, strlen(http_prefix)) == 0) {
    url += strlen(http_prefix);
  }

  size_t url_length = strlen(url);

  if (strncmp(url, www_prefix, strlen(www_prefix)) == 0) {
    url += strlen(www_prefix);
  }

  const char* slash_position = strchr(url, '/');
  if (slash_position != NULL) {
    url_length = slash_position - url;
  }

  char* modified_url = (char*)malloc(url_length + 1);
  strncpy(modified_url, url, url_length);
  modified_url[url_length] = '\0';

  return modified_url;
}

void usage(const char* run)
{
  puts("LastTrench: Finding what the person left behind online.\n");

  printf("usage: %s <name, name1.../)> [flags]\n\n", run);

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

int send_http_request(const char* ip, const int port, const int timeout_ms,
    const struct http_header* header, char* response_buffer, size_t buffer_size)
{
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  inet_pton(AF_INET, ip, &server_addr.sin_addr);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("send_http_request/create_socket");
    return -1;
  }

  struct timeval timeout;
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) == -1) {
    perror("send_http_request/set_timeout");
    close(sockfd);
    return -1;
  }

  if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("send_http_request/connect_to_host");
    close(sockfd);
    return -1;
  }

  char request[1024];
  snprintf(request, sizeof(request),
    "%s %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %lu\r\n"
    "Connection: close\r\n"
    "Authorization: Basic %s\r\n\r\n",

  header->method,
  header->path, header->dest_host, header->user_agent, header->content_type, header->content_len, header->auth_header);

  int send_req = send(sockfd, request, strlen(request), 0);
  if (send_req == -1) {
    perror("send_http_request/send_packet");
    close(sockfd);
    return -1;
  }

  size_t response_length = 0;
  ssize_t bytes_received;

  while ((bytes_received = recv(sockfd, response_buffer + response_length, buffer_size - response_length - 1, 0)) > 0) {
    response_length += bytes_received;
  }

  response_buffer[response_length] = '\0';

  close(sockfd);
  return response_length;
}

int parse_http_response_code(const char* http_content)
{
  int code = -1;
  const char* status = strstr(http_content, "HTTP/1.");
  if (status != NULL) {
    sscanf(status, "HTTP/1.%*d %d", &code);
  }

  return code;
}

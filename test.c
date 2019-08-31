#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <unistd.h>
#include <signal.h>
#include "core/constants.h"

#define DEFAULT_TIMEOUT 50000

#define CONFIG_CONTENT_BACK "\
server_port:7070\n\
server_type:"

#define CONFIG_CONTENT_NEW "\
server_port:7071\n\
server_type:"

#define SIMPLE_TEXT "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;
  return size*nmemb;
}

int main(int argc, char *argv[]) {
  FILE *info = fopen(INFO_PATH,"r");
  if(info == NULL) {
      fprintf(stderr,"Impossibile leggere il file di infos.\n");
      return -1;
  }

  char line[50];
  if(fgets(line,50,info) < 0) {
      fprintf(stderr, "Impossibile leggere il file di infos. (getline).\n");
      return - 1;
  }

  char *pid_str = strchr(line, ':')+1;
  int size = pid_str-line-1;
  char type[size];
  memcpy(type,line, size);
  int pid = strtol(pid_str, NULL, 10);

  CURL *curl;
  fclose(info);
  int passed = 0;
  int failed = 0;

  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "%s\n", "Errore: init curl");
    curl_easy_cleanup(curl);
    exit(0);
  }
  struct string s;
  init_string(&s);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

  char string[64];
/*#----------------------------------  // TEST_1  ---------------------------------------#*/
  char *desc = "Verifica che venga fatto correttamente il listing delle cartella root";
  curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/");

  if (curl_easy_perform(curl) == 0) {
    passed++;
    fprintf(stdout, "#PASSED: %s\n", desc);
  }
  else {
    failed++;
    fprintf(stdout, "#FAILED: %s\n", desc);
  }
  init_string(&s);
  usleep(DEFAULT_TIMEOUT);

/*#----------------------------------  // TEST_2  ---------------------------------------#*/
  desc = "Verifica che venga fatto correttamente il listing di una sottocartella";
  curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/folder");

  if (curl_easy_perform(curl) == 0) {
      passed++;
      fprintf(stdout, "#PASSED: %s\n", desc);
  }
  else {
      failed++;
      fprintf(stdout, "#FAILED: %s\n", desc);
  }
  init_string(&s);
  usleep(DEFAULT_TIMEOUT);

/*#----------------------------------  // TEST_3  ---------------------------------------#*/
  desc = "Verifica che venga correttamente restituito un file .txt";
  curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/testo.txt");

  if (curl_easy_perform(curl) == 0) {
    passed++;
    fprintf(stdout, "#PASSED: %s\n", desc);
  }
  else {
    failed++;
    fprintf(stdout, "#FAILED: %s\n", desc);
  }
  init_string(&s);
  usleep(DEFAULT_TIMEOUT);

/*#----------------------------------  // TEST_4  ---------------------------------------#*/
  desc = "Verifica che venga correttamente restituito un file vuoto .txt";
  curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/folder/vuoto.txt");

  if (curl_easy_perform(curl) == 0) {
    passed++;
    fprintf(stdout, "#PASSED: %s\n", desc);
  }
  else {
    failed++;
    fprintf(stdout, "#FAILED: %s\n", desc);
  }
  init_string(&s);
  usleep(DEFAULT_TIMEOUT);

/*#----------------------------------  // TEST_5  ---------------------------------------#*/
  desc = "Verifica che il server cambi porta con SIGHUP";
  FILE *file = fopen(CONFIG_PATH, "w+");
  sprintf(string,"%s%s",CONFIG_CONTENT_NEW,type);
  fprintf(file, "%s", string);
  fclose(file);
  kill(pid, SIGHUP);

  curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7071/1/testo.txt");

  if (curl_easy_perform(curl) == 0) {
    passed++;
    fprintf(stdout, "#PASSED: %s\n", desc);
  }
  else {
    failed++;
    fprintf(stdout, "#FAILED: %s\n", desc);
  }
  init_string(&s);
  file = fopen(CONFIG_PATH, "w+");
  sprintf(string,"%s%s",CONFIG_CONTENT_BACK,type);
  fprintf(file, "%s", string);
  fclose(file);
  usleep(DEFAULT_TIMEOUT * 20);

/*#----------------------------------  // TEST_6  ---------------------------------------#*/
  desc = "Verifica che il server non ascolti più sulla porta precedente";
  curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/testo.txt");

  if (curl_easy_perform(curl) != 0) {
    passed++;
    fprintf(stdout, "#PASSED: %s\n", desc);
  }
  else {
    failed++;
    fprintf(stdout, "#FAILED: %s\n", desc);
  }
  init_string(&s);
  usleep(DEFAULT_TIMEOUT);

/*#----------------------------------  // TEST_BOMB  ---------------------------------------#*/

  desc = "Verifica che il server riesca a gestire multiple richieste consecutive";
  int lower = 1000;
  int upper = 5000;
  int bomb = 1;
  puts("#TEST_BOMB: Please wait");
  for (int i = 0; i < 10000; i++) {
      if (i % 1000 == 0) {
          printf(".");
          fflush(stdout);
      }
      curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7071/0/testo.txt");
      int res = curl_easy_perform(curl);
      if (res != 0 && strcmp(s.ptr,SIMPLE_TEXT) != 0) {
          bomb = 0;
          break;
      }
      init_string(&s);
      int timeout = (rand() % (upper - lower + 1)) + lower;
      usleep(timeout);
  }
  if (bomb == 1) {
      fprintf(stdout, "\n#PASSED: %s\n", desc);
  } else {
      fprintf(stdout, "\n#FAILED: %s\n", desc);
  }
  int total = passed + failed + 1;

  fprintf(stdout, "\n%s%d%s%d\n", "Numero di test passati: ", passed + bomb, "/", total);

  free(s.ptr);
  curl_easy_cleanup(curl);
  system("pkill -f ps_server");
  return 0;
}

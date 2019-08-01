#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <unistd.h>
#define HALF_SECOND 5 * 100000
#define PID_FLAG "--pid="
#define HELP_FLAG "--help"
#define equals(str1, str2) strcmp(str1,str2) == 0

#define ROOT_LISTING "\
g anim.gif\t/\t127.0.0.1\t7070\n\
1 folder\t/\t127.0.0.1\t7070\n\
0 testo.txt\t/\t127.0.0.1\t7070\n\
I image.jpg\t/\t127.0.0.1\t7070\n\
.\n"

#define SUBFOLDER_LISTING "\
0 vuoto.txt\t/folder/\t127.0.0.1\t7070\n\
.\n"

#define SIMPLE_TEXT "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"

#define EMPTY_STRING "\n"

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
  int pid = 0;
  char *input;
  for (int i = 1; i < argc; i++) {
      input = argv[i];
      char *endptr;
      if (strncmp(input, PID_FLAG, strlen(PID_FLAG)) == 0) {
          pid = strtol(input + strlen(PID_FLAG), &endptr, 10);
      } else if (strncmp(input, HELP_FLAG, strlen(HELP_FLAG)) == 0) {
          puts("Tool per testare le funzionalità del server.\n\n"
               "--pid=<pid_server> per specificare il process id del server da testare.\n");
          exit(0);
      }
  }

  if (pid == 0) {
    fprintf(stderr, "%s\n", "Errore: verificare che il PID sia stato inserito correttamente.");
    exit(1);
  }

  CURL *curl;
  CURLcode res;

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


  // TEST_1: Verifica che venga fatto correttamente il listing delle cartella root
  curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/");
  res = curl_easy_perform(curl);
  if (equals(s.ptr, ROOT_LISTING)) passed++;
  else {
    failed++;
    fprintf(stderr, "%s\n", "#FAIL: 'Verifica che venga fatto correttamente il listing delle cartella root'");
  }
  init_string(&s);
  usleep(HALF_SECOND);

  // TEST_2: Verifica che venga fatto correttamente il listing di una sottocartella
  curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/folder");
  res = curl_easy_perform(curl);
  if (equals(s.ptr, SUBFOLDER_LISTING)) passed++;
  else {
    failed++;
    fprintf(stderr, "%s\n", "#FAIL: 'Verifica che venga fatto correttamente il listing di una sottocartella'");
  }
  init_string(&s);
  usleep(HALF_SECOND);

  // TEST_3: Verifica che venga correttamente restituito un file .txt
  curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/testo.txt");
  res = curl_easy_perform(curl);
  if (equals(s.ptr, SIMPLE_TEXT)) passed++;
  else {
    failed++;
    fprintf(stderr, "%s\n", "#FAIL: 'Verifica che venga correttamente restituito un file .txt'");
  }
  init_string(&s);
  usleep(HALF_SECOND);

  // TEST_4: Verifica che venga correttamente restituito un file vuoto .txt
  curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/folder/vuoto.txt");
  res = curl_easy_perform(curl);
  if (equals(s.ptr, EMPTY_STRING)) passed++;
  else {
    failed++;
    fprintf(stderr, "%s\n", "#FAIL: 'Verifica che venga correttamente restituito un file vuoto .txt'");
  }
  init_string(&s);
  //usleep(HALF_SECOND);

  int total = passed + failed;

  fprintf(stdout, "\n%s%d%s%d\n", "Numero di test passati: ", passed, "/", total);

  free(s.ptr);
  curl_easy_cleanup(curl);

  return 0;
}

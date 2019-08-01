#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

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

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
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

int main(void)
{
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
    struct string s;
    init_string(&s);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    // TEST_1: Verificare che venga fatto correttamente il listing delle cartella root
    curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/");
    res = curl_easy_perform(curl);
    printf("%s\n", s.ptr);
    init_string(&s);

    // TEST_2: Verificare che venga fatto correttamente il listing di una sottocartella
    curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/folder");
    res = curl_easy_perform(curl);
    printf("%s\n", s.ptr);
    init_string(&s);

    // TEST_3: Verificare che venga correttamente restituito un file .txt
    curl_easy_setopt(curl, CURLOPT_URL, "gopher://localhost:7070/1/text.a.txt");
    res = curl_easy_perform(curl);
    printf("%s\n", s.ptr);
    init_string(&s);



    free(s.ptr);
    curl_easy_cleanup(curl);
  }
  return 0;
}

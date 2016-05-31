#include "stemmer.h"
#include <string.h>
#include <stdio.h>
#include "dep/snowball/include/libstemmer.h"

const char *__sbstemmer_Stem(void *ctx, const char *word, size_t len,
                             size_t *outlen) {
  const sb_symbol *b = (const sb_symbol *)word;
  struct sb_stemmer *sb = ctx;

  const sb_symbol *stemmed = sb_stemmer_stem(sb, b, (int)len);
  if (stemmed) {
    *outlen = sb_stemmer_length(sb);
    return (const char *)stemmed;
  }
  return NULL;
}

void __sbstemmer_Free(Stemmer *s) {
  sb_stemmer_delete(s->ctx);
  free(s);
}

Stemmer *__newSnowballStemmer(const char *language) {
  struct sb_stemmer *sb = sb_stemmer_new(language, NULL);
  // No stemmer available for this language
  if (!sb) {
    return NULL;
  }

  Stemmer *ret = malloc(sizeof(Stemmer));
  ret->ctx = sb;
  ret->Stem = __sbstemmer_Stem;
  ret->Free = __sbstemmer_Free;
  return ret;
}

Stemmer *NewStemmer(StemmerType type, const char *language) {
  switch (type) {
    case SnowballStemmer:

      return __newSnowballStemmer(language);
  }

  fprintf(stderr, "Invalid stemmer type");
  return NULL;
}

#include "test.h"
#include "query.h"
#include "query-parser.h"

TEST(empty_queries) {
  wp_query* q;
  RELAY_ERROR(wp_query_parse("", "body", &q));
  ASSERT(q);

  return NO_ERROR;
}

/* most of the query parsing code is exercised by the
   search tests, so this is just for fun. */
TEST(query_parsing) {
  wp_query* q;
  RELAY_ERROR(wp_query_parse("i eat mice", "body", &q));
  ASSERT_EQUALS_UINT(3, q->num_children);
  ASSERT_EQUALS_UINT(WP_QUERY_CONJ, q->type);
  ASSERT_EQUALS_PTR(NULL, q->field);
  ASSERT_EQUALS_PTR(NULL, q->word);

  ASSERT(q->children != NULL);
  ASSERT_EQUALS_UINT(WP_QUERY_TERM, q->children->type);
  ASSERT(!strcmp(q->children->field, "body"));
  ASSERT(!strcmp(q->children->word, "i"));

  return NO_ERROR;
}

TEST(query_cloning) {
  wp_query* q;
  RELAY_ERROR(wp_query_parse("i eat mice OR \"muffin pants\" bob:pumpkin", "body", &q));

  char buf[100];
  wp_query_to_s(q, 100, buf);
  ASSERT(!strcmp(buf, "(AND body:\"i\" body:\"eat\" (OR body:\"mice\" (PHRASE body:\"muffin\" body:\"pants\")) bob:\"pumpkin\")"));

  wp_query* q2;
  RELAY_ERROR(wp_query_parse("i eat mice OR \"muffin pants\" bob:pumpkin", "body", &q));
  q2 = wp_query_clone(q);
  ASSERT(q2);
  ASSERT(q2 != q);

  wp_query_to_s(q2, 100, buf);
  ASSERT(!strcmp(buf, "(AND body:\"i\" body:\"eat\" (OR body:\"mice\" (PHRASE body:\"muffin\" body:\"pants\")) bob:\"pumpkin\")"));

  return NO_ERROR;
}

static const char* substituter(const char* field, const char* term) {
  (void)field;
  char* ret = calloc(strlen(term) + 3, sizeof(char));
  sprintf(ret, "X%sY", term);
  return ret;
}

TEST(query_substitution) {
  wp_query* q;
  RELAY_ERROR(wp_query_parse("i eat mice", "body", &q));

  wp_query* q2;
  q2 = wp_query_substitute(q, substituter);
  ASSERT(q2);
  ASSERT(q2 != q);

  char buf[100];
  wp_query_to_s(q2, 100, buf);
  ASSERT(!strcmp(buf, "(AND body:\"XiY\" body:\"XeatY\" body:\"XmiceY\")"));

  RELAY_ERROR(wp_query_parse("i eat ~mice", "body", &q));
  q2 = wp_query_substitute(q, substituter);
  ASSERT(q2);
  ASSERT(q2 != q);

  wp_query_to_s(q2, 100, buf);
  ASSERT(!strcmp(buf, "(AND body:\"XiY\" body:\"XeatY\" ~mice)"));

  return NO_ERROR;
}

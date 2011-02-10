#include <stdio.h>
#include "whistlepig.h"

#define isempty(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&2)
#define isdel(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&1)
#define KEY(h, i) &(h->pool[h->keys[i]])

RAISING_STATIC(dump_posting_list(wp_segment* s, uint32_t offset)) {
  posting po;

  while(offset != OFFSET_NONE) {
    RELAY_ERROR(wp_segment_read_posting(s, offset, &po, 1));

    printf("  @%u doc %u:", offset, po.doc_id);
    for(uint32_t i = 0; i < po.num_positions; i++) {
      printf(" %d", po.positions[i]);
    }
    printf("\n");

    offset = po.next_offset;
    free(po.positions);
  }

  return NO_ERROR;
}

RAISING_STATIC(dump(wp_segment* segment)) {
  termhash* th = MMAP_OBJ(segment->termhash, termhash);
  stringmap* sh = MMAP_OBJ(segment->stringmap, stringmap);

  for(uint32_t i = 0; i < th->n_buckets; i++) {
    if(isempty(th->flags, i)); // do nothing
    else if(isdel(th->flags, i)) printf("%u: [deleted]", i);
    else {
      term t = th->keys[i];
      const char* field = stringmap_int_to_string(sh, t.field_s);
      const char* word = stringmap_int_to_string(sh, t.word_s);
      printf("%u: %s:'%s'\n", i, field, word);
      RELAY_ERROR(dump_posting_list(segment, th->vals[i]));
    }
  }

  return NO_ERROR;
}

int main(int argc, char* argv[]) {
  if(argc != 2) {
    fprintf(stderr, "Usage: %s <segment filename>\n", argv[0]);
    return -1;
  }

  wp_index* index;
  DIE_IF_ERROR(wp_index_load(&index, argv[1]));
  DIE_IF_ERROR(wp_index_dumpinfo(index, stdout));

  for(int i = 0; i < index->num_segments; i++) {
    printf("\nsegment %d details:\n", i);
    DIE_IF_ERROR(dump(&index->segments[i]));
  }

  DIE_IF_ERROR(wp_index_unload(index));

  return 0;
}

#include "rs_hash.h"
#include "config.h"
#include "rmutil/rm_assert.h"

extern bool isCrdt;

typedef struct {
  RedisModuleCtx *ctx;
  SchemaRule *rule;
  arrayof(RedisModuleString *) arr;
} HashPrintArgs;

static void Hash_Cursor_cb(RedisModuleKey *key, RedisModuleString *field, RedisModuleString *value, void *privdata) {
  REDISMODULE_NOT_USED(key);
  HashPrintArgs *args = privdata;
  arrayof(RedisModuleString *) arr = args->arr;
  RedisModuleCtx *ctx = args->ctx;
  SchemaRule *rule = args->rule;
  
  // Do not reply with these fields
  const char *fieldStr = RedisModule_StringPtrLen(field, NULL);

  
  if (rule && ((rule->lang_field && strcmp(fieldStr, rule->lang_field) == 0) ||
               (rule->score_field && strcmp(fieldStr, rule->score_field) == 0) ||
               (rule->payload_field && strcmp(fieldStr, rule->score_field) == 0))) {
    return;
  }

  RedisModule_RetainString(ctx, field);
  RedisModule_RetainString(ctx, value);
  arr = array_ensure_append(arr, &field, 1, RedisModuleString *);
  arr = array_ensure_append(arr, &value, 1, RedisModuleString *);
}

int RS_ReplyWithHash(RedisModuleCtx *ctx, char *keyC, arrayof(RedisModuleString *) replyArr, SchemaRule *rule) {
  // Reply using scanning - from Redis 6.0.6
  if (isFeatureSupported(RM_SCAN_KEY_API_FIX) && !isCrdt) {
    int rc = REDISMODULE_ERR;
    RedisModuleString *keyR = RedisModule_CreateString(ctx, keyC, strlen(keyC));
    RedisModuleKey *key = RedisModule_OpenKey(ctx, keyR, REDISMODULE_READ);
    if (!key || RedisModule_KeyType(key) != REDISMODULE_KEYTYPE_HASH) {
      goto done;
    }

    array_clear(replyArr);
    HashPrintArgs args = { .ctx = ctx,
                           .arr = replyArr,
                           .rule = rule, };

    RedisModuleScanCursor *cursor = RedisModule_ScanCursorCreate();
    while(RedisModule_ScanKey(key, cursor, Hash_Cursor_cb, &args));
    RedisModule_ScanCursorDestroy(cursor);

    int len = array_len(replyArr);
    RedisModule_ReplyWithArray(ctx, len);
    for (uint32_t i = 0; i < array_len(replyArr); i++) {
      RedisModuleString *reply = replyArr[i];
      RedisModule_ReplyWithString(ctx, reply);
      RedisModule_FreeString(ctx, reply);
      replyArr[i] = NULL;
    } 

    rc = REDISMODULE_OK;
  done:
    if (key) RedisModule_CloseKey(key);
    if (keyR) RedisModule_FreeString(ctx, keyR);
    return rc;
  // Reply using RM_Call()
  } else {
    RedisModuleCallReply *reply = RedisModule_Call(ctx, "HGETALL", "c", keyC);
    if (!reply || RedisModule_CallReplyType(reply) != REDISMODULE_REPLY_ARRAY) {
      RedisModule_ReplyWithNull(ctx);
      return REDISMODULE_ERR;
    }
    RedisModule_ReplyWithCallReply(ctx, reply);
    RedisModule_FreeCallReply(reply);
    //RS_LOG_ASSERT(0, "Remove");
    return REDISMODULE_OK;
  }
}
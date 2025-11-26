// json_serialize.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__has_include)
#  if __has_include(<cjson/cJSON.h>)
#    include <cjson/cJSON.h>
#  elif __has_include(<cJSON.h>)
#    include <cJSON.h>
#  else
#    error "cJSON.h not found. Install cJSON or add include path."
#  endif
#else
#  include <cjson/cJSON.h>
#endif

// Mason
int main(int argc, char **argv) {
    
    // arbitrary data
    const char *user = (argc > 1) ? argv[1] : "test";
    int session_id = (argc > 2) ? atoi(argv[2]) : 42;
    int valid = (argc > 3) ? atoi(argv[3]) : 1;
    const char *command = (argc > 4) ? argv[4] : "move";
    const char *extra = (argc > 5) ? argv[5] : "placeholder";
    int nonce = (argc > 6) ? atoi(argv[6]) : 1173489;

    cJSON *root = cJSON_CreateObject();
    if (!root) { fprintf(stderr, "cJSON_CreateObject failed\n"); return 1; }

    cJSON_AddStringToObject(root, "user", user);
    cJSON_AddNumberToObject(root, "id", session_id);
    cJSON_AddBoolToObject(root, "valid", valid);
    cJSON_AddStringToObject(root, "command", command);
    cJSON_AddStringToObject(root, "extra", extra);
    cJSON_AddNumberToObject(root, "nonce", nonce);

    // serialize to string
    char *json = cJSON_PrintUnformatted(root);
    if (!json) { cJSON_Delete(root); fprintf(stderr, "cJSON_PrintUnformatted failed\n"); return 1; }

    puts(json);

    free(json);
    cJSON_Delete(root);
    return 0;
}

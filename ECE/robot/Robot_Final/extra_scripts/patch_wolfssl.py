import shutil
import os
import glob

Import("env")

#Step 1 - Copy correct user_settings.h
src = os.path.join(env["PROJECT_DIR"], "include", "user_settings.h")
dst = os.path.join(env["PROJECT_DIR"], ".pio", "libdeps", "esp32dev",
                   "wolfssl", "wolfssl", "user_settings.h")

if os.path.exists(src):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(src, dst)
    print(">>> Patched wolfSSL user_settings.h successfully")
else:
    print(">>> WARNING: source user_settings.h not found at", src)

#Step 2 - Delete cached libwolfssl.a to force recompile
build_dir = os.path.join(env["PROJECT_DIR"], ".pio", "build", "esp32dev")
for cached_lib in glob.glob(os.path.join(build_dir, "*", "libwolfssl.a")):
    os.remove(cached_lib)
    print(">>> Deleted cached libwolfssl.a at", cached_lib)
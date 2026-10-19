#!/bin/sh
# Host test of darwin_path() (source/darwin.c): relative paths against the working directory,
# "dir/../" collapsing and Documents -> saves/.
# Uso: sh tools/test_darwin_path.sh
set -e
D=$(mktemp -d)
trap 'rm -rf "$D"' EXIT
cd "$(dirname "$0")/.."
{
    printf '#include <assert.h>\n#include <stdio.h>\n#include <string.h>\n'
    printf '#define DATA_PATH "ux0:data/skaternation/"\n#define DOCS_PATH DATA_PATH "saves/"\n'
    printf '#define BUNDLE_PATH DATA_PATH "SkateNation.app/"\n'
    sed -n '/^static char cwd\[/,/^}/p' source/darwin.c
    sed -n '/^const char \*darwin_path/,/^}/p' source/darwin.c
    cat <<'C'
static const char *t(const char *in) { static char o[512]; return darwin_path(in, o, sizeof(o)); }
int main(void) {
    assert(!strcmp(t("ux0:data/skaternation/SkateNation.app/../Documents/saveSettings.dat"),
                   "ux0:data/skaternation/saves/saveSettings.dat"));
    assert(!strcmp(t("ux0:data/skaternation/saves/profile.sav"), "ux0:data/skaternation/saves/profile.sav"));
    assert(!strcmp(t("/var/mobile/x.dat"), "ux0:data/skaternation/var/mobile/x.dat"));
    assert(!strcmp(t("ux0:data/skaternation/Documents"), "ux0:data/skaternation/saves/"));
    assert(!strcmp(t("ux0:data/skaternation/SkateNation.app/data.pack"),
                   "ux0:data/skaternation/SkateNation.app/data.pack"));
    assert(!strcmp(t("data.pack"), "ux0:data/skaternation/SkateNation.app/data.pack"));
    assert(!strcmp(t("./hud.bsprite"), "ux0:data/skaternation/SkateNation.app/hud.bsprite"));
    assert(!strcmp(t("../Documents/saveProgress.dat"), "ux0:data/skaternation/saves/saveProgress.dat"));
    puts("darwin_path ok");
    return 0;
}
C
} > "$D/t.c"
cc -o "$D/t" "$D/t.c" && "$D/t"

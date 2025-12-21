#set($dir = "${DIR_PATH}")
#set($hdr = "${HEADER_FILENAME}")

#if ($dir && $dir.trim().length() > 0)
#[[#include]]# "$dir/$hdr"
#else
#[[#include]]# "$hdr"
#end

${NAMESPACES_OPEN}


${NAMESPACES_CLOSE}
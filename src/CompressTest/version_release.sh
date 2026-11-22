set -e
"$CLOUD_REBUILD" Compress 64 lib release same
"$CLOUD_REBUILD" PlatformTest 64 lib release same
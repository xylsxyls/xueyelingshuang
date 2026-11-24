set -e

"$CLOUD_REBUILD" NetInterface 32 lib debug same
"$CLOUD_REBUILD" PlatformTest 32 lib debug same

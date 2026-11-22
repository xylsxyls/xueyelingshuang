set -e
"$CLOUD_REBUILD" CDump "${1:-64}" lib release same
"$CLOUD_REBUILD" PlatformTest "${1:-64}" lib release same
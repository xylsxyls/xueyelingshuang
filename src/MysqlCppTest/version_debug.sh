"$CLOUD_REBUILD" MysqlCpp "${1:-32}" lib debug same
mysqlcpp_build_status=$?
if [ "$mysqlcpp_build_status" -ne 0 ]; then
    exit "$mysqlcpp_build_status"
fi
"$CLOUD_REBUILD" PlatformTest "${1:-32}" dll debug same
#设置CMAKE最低版本
CMAKE_MINIMUM_REQUIRED(VERSION 3.1.1)
#设置项目名称
SET(PROJECT_NAME stream_ingress_service)
SET(CMAKE_CXX_STANDARD 11)

#建立项目
PROJECT(${PROJECT_NAME})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/)
#SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include/)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/base_components/core/include/)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/base_components/core/thirdparty/include/)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/base_components/multi_video_service/include/)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/base_components/multi_video_service/thirdparty/include/)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/base_components/sys_logger/include/)
#INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/thirdparty/include/)
#获取代码，在项目中，将所有代码都放在src文件夹中
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/source/ DIR_SRCS)
#编译可执行程序
ADD_EXECUTABLE(${PROJECT_NAME} ${DIR_SRCS})
#如果要生成静态链接库
#ADD_LIBRARY(${PROJECT_NAME} STATIC ${DIR_SRCS})
#如果要生成动态链接库
#ADD_LIBRARY(${PROJECT_NAME} SHARED ${DIR_SRCS})

#添加链接库
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/base_components/core/lib/)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/base_components/core/thirdparty/libs/)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/base_components/multi_video_service/lib/)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/base_components/multi_video_service/thirdparty/libs/)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/base_components/sys_logger/lib/)

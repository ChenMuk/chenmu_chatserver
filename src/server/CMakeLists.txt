
#添加目录下所有的cpp文件到自定义的变量SRC_LIST
aux_source_directory(. SRC_LIST)
aux_source_directory(./model SRC_LIST)
aux_source_directory(./db MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

#生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

#链接库 -lmuduo_net -lmuduo_base -lpthread
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)
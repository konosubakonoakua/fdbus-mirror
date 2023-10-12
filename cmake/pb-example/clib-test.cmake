link_libraries(fdbus-clib)

add_executable(fdbtestclibserver
    ${PACKAGE_SOURCE_ROOT}/c/test/fdbus_test_server.c
)

add_executable(fdbtestclibclient
    ${PACKAGE_SOURCE_ROOT}/c/test/fdbus_test_client.c
)

add_executable(fdbtestclibappfwserver
    ${PACKAGE_SOURCE_ROOT}/c/test/fdbus_test_afcomponent_server.c
)

add_executable(fdbtestclibappfwclient
    ${PACKAGE_SOURCE_ROOT}/c/test/fdbus_test_afcomponent_client.c
)

add_executable(fdbtestclibdatapool
    ${PACKAGE_SOURCE_ROOT}/c/test/fdbus_test_datapool.c
)

install(TARGETS fdbtestclibserver fdbtestclibclient fdbtestclibappfwserver fdbtestclibappfwclient fdbtestclibdatapool RUNTIME DESTINATION usr/bin)

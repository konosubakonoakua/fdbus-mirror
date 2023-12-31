link_libraries(fdbus)

add_executable(name_server
    ${PACKAGE_SOURCE_ROOT}/server/main_ns.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CNameServer.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CInterNameProxy.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CIntraHostProxy.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CNameProxyContainer.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CBaseHostProxy.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CAddressAllocator.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CSvcAddrUtils.cpp
    ${PACKAGE_SOURCE_ROOT}/security/CServerSecurityConfig.cpp
)

add_executable(host_server
    ${PACKAGE_SOURCE_ROOT}/server/main_hs.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CHostServer.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CBaseHostProxy.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CInterHostProxy.cpp
    ${PACKAGE_SOURCE_ROOT}/server/CSvcAddrUtils.cpp
    ${PACKAGE_SOURCE_ROOT}/security/CHostSecurityConfig.cpp
)

add_executable(lssvc
    ${PACKAGE_SOURCE_ROOT}/server/main_ls.cpp
)

add_executable(lshost
    ${PACKAGE_SOURCE_ROOT}/server/main_lh.cpp
)

add_executable(lsclt
    ${PACKAGE_SOURCE_ROOT}/server/main_lc.cpp
)

add_executable(logsvc
    ${PACKAGE_SOURCE_ROOT}/log/main_log_server.cpp
    ${PACKAGE_SOURCE_ROOT}/log/CLogFileManager.cpp
    ${PACKAGE_SOURCE_ROOT}/log/fdb_log_config.cpp
)

add_executable(logviewer
    ${PACKAGE_SOURCE_ROOT}/log/main_log_client.cpp
    ${PACKAGE_SOURCE_ROOT}/log/fdb_log_config.cpp
)

add_executable(fdbxclient
    ${PACKAGE_SOURCE_ROOT}/server/main_xclient.cpp
)

add_executable(fdbxserver
    ${PACKAGE_SOURCE_ROOT}/server/main_xserver.cpp
)

add_executable(ntfcenter
    ${PACKAGE_SOURCE_ROOT}/server/main_nc.cpp
)

add_executable(lsevt
    ${PACKAGE_SOURCE_ROOT}/server/main_le.cpp
)

add_executable(lsdp
    ${PACKAGE_SOURCE_ROOT}/server/main_ld.cpp
)

install(TARGETS name_server host_server lssvc lshost lsclt logsvc logviewer fdbxclient fdbxserver ntfcenter lsevt lsdp RUNTIME DESTINATION usr/bin)

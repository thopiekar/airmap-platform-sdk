add_executable(
    monitor.client 

    $<TARGET_OBJECTS:airmap-client>

    client.cpp)

set_property(TARGET monitor.client PROPERTY CXX_STANDARD 17)

target_link_libraries(
  monitor.client

  airmap-monitor

  ${Boost_LIBRARIES}
  ${OPENSSL_LDFLAGS}
)
add_executable(
    monitor.client 

    $<TARGET_OBJECTS:airmap-client>

    client.cpp)

set_property(TARGET monitor.client PROPERTY CXX_STANDARD 14)

target_link_libraries(
  monitor.client

  airmap-monitor

  Boost::date_time
  Boost::filesystem
  Boost::log
  Boost::program_options
  Boost::system
  Boost::thread

  ${OPENSSL_LDFLAGS}
)

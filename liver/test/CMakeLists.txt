
add_executable(fill_test fill_liver_test.cpp)
dmip_setup_target(fill_test "../..")

add_test(NAME fill_tester COMMAND fill_test)

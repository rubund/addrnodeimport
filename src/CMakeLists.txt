find_package(PkgConfig)

pkg_check_modules(XML libxml-2.0)


if (XML_FOUND)
    add_executable(getedges getedges.c)
    add_executable(getmissingandreport getmissingandreport.c)

    install(TARGETS getedges DESTINATION bin)
    install(TARGETS getmissingandreport DESTINATION bin)

    target_link_libraries(getedges ${XML_LIBRARIES})
    target_link_libraries(getmissingandreport ${XML_LIBRARIES})
    include_directories(${XML_INCLUDE_DIRS})
else (XML_FOUND)
    MESSAGE( "Missing one more packages for building gui. Skipping")
endif(XML_FOUND)


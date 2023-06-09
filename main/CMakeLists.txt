file(GLOB U_MODULES "modules/*")

idf_component_register( SRCS

    main.cpp
    eeprom.cpp

INCLUDE_DIRS ".." "include")

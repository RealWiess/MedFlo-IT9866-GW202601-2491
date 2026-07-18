SET (BARROT_LIB_PATH
	${PROJECT_SOURCE_DIR}/sdk/share/barrot/libbarrot.a
)

file(COPY ${BARROT_LIB_PATH}
        DESTINATION ${CMAKE_BINARY_DIR}/lib/${CFG_CPU_NAME}/
    )
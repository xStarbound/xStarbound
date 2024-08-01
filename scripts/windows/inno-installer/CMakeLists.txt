
configure_file(xsb-installer.iss.in ${CMAKE_BINARY_DIR}/inno-installer/xsb-installer.iss @ONLY)

file(COPY
	ChineseSimplified.isl
	ChineseTraditional.isl
	English.isl
	French.isl
	German.isl
	install-info.txt
	Russian.isl
	Spanish.isl
	DESTINATION "${CMAKE_BINARY_DIR}/inno-installer/"
)

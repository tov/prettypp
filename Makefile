doc: Doxyfile src/*.h src/detail/*.h
	doxygen

upload-doc:
	make doc
	ghp-import -n doc/html
	git push -f https://github.com/tov/weakpp.git gh-pages

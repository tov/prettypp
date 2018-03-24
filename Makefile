doc: Doxyfile src/*.h
	doxygen

upload-doc:
	make doc
	ghp-import -n doc/html
	git push -f https://github.com/tov/prettypp.git gh-pages

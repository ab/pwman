EXTRA_DIST = pwman.1

install-data-local:
	        $(mkinstalldirs) $(DESTDIR)$(mandir)/man1
		$(INSTALL_DATA) $(srcdir)/pwman.1 $(DESTDIR)$(mandir)/man1/

uninstall-local:
	        -rm -f $(DESTDIR)$(mandir)/man1/pwman.1
